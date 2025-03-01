#include "object_renderer.hpp"

#include "render/mesh/shader/scene_data.h"
#include "render/render_server.hpp"
#include "scene/camera.hpp"
#include "scene/components/components.hpp"
#include "scene/components/transform.hpp"
#include "scene/scene.hpp"

#include "render_state.hpp"

#include <vke/pipeline_loader.hpp>
#include <vke/util.hpp>
#include <vke/vke.hpp>
#include <vke/vke_builders.hpp>

#include <vk_mem_alloc.h>

namespace vke {

struct InstanceComponent {
    const ObjectRenderer::InstanceID id;
};

ObjectRenderer::ObjectRenderer(RenderServer* render_server) {
    m_render_server   = render_server;
    m_descriptor_pool = std::make_unique<DescriptorPool>();

    m_dummy_buffer = std::make_unique<vke::Buffer>(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, 256, false);

    {
        vke::DescriptorSetLayoutBuilder layout_builder;
        layout_builder.add_ssbo(VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT); // lights
        auto stages = VK_SHADER_STAGE_ALL;
        layout_builder.add_ssbo(stages); // instances
        layout_builder.add_ssbo(stages); // models
        layout_builder.add_ssbo(stages); // parts
        layout_builder.add_ssbo(stages); // meshes

        m_scene_set_layout = layout_builder.build();
    }

    {
        vke::DescriptorSetLayoutBuilder layout_builder;
        layout_builder.add_ubo(VK_SHADER_STAGE_ALL); // scene_view

        layout_builder.add_ssbo(VK_SHADER_STAGE_COMPUTE_BIT); // instance_draw_parameter_locations
        layout_builder.add_ssbo(VK_SHADER_STAGE_COMPUTE_BIT); // instance_counters
        layout_builder.add_ssbo(VK_SHADER_STAGE_COMPUTE_BIT); // indirect_draw_locations
        layout_builder.add_ssbo(VK_SHADER_STAGE_COMPUTE_BIT); // draw_commands
        layout_builder.add_ssbo(VK_SHADER_STAGE_ALL);         // instance_draw_parameters

        m_view_set_layout = layout_builder.build();
    }

    auto pg_provider = m_render_server->get_pipeline_loader()->get_pipeline_globals_provider();

    m_resource_manager = std::make_unique<ResourceManager>(render_server);

    pg_provider->set_layouts["vke::object_renderer::material_set"] = m_resource_manager->get_material_set_layout();
    pg_provider->set_layouts["vke::object_renderer::scene_set"]    = m_scene_set_layout;
    pg_provider->set_layouts["vke::object_renderer::view_set"]     = m_view_set_layout;

    for (auto& framely : m_framely_data) {
        framely.light_buffer = std::make_unique<Buffer>(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, sizeof(SceneLightData), true);
        vke::DescriptorSetBuilder builder;
        builder.add_ssbo(framely.light_buffer.get(), VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT);

        auto stages = VK_SHADER_STAGE_ALL;
        if (m_scene_data) {
            builder.add_ssbo(m_scene_data->instance_buffer.get(), stages);
            builder.add_ssbo(m_scene_data->model_info_buffer.get(), stages);
            builder.add_ssbo(m_scene_data->model_part_info_buffer.get(), stages);
            builder.add_ssbo(m_scene_data->mesh_info_buffer.get(), stages);
        } else {
            builder.add_ssbo(m_dummy_buffer.get(), stages);
            builder.add_ssbo(m_dummy_buffer.get(), stages);
            builder.add_ssbo(m_dummy_buffer.get(), stages);
            builder.add_ssbo(m_dummy_buffer.get(), stages);
        }

        framely.scene_set = builder.build(m_descriptor_pool.get(), m_scene_set_layout);
    }
}

ObjectRenderer::~ObjectRenderer() {
    vkDestroyDescriptorSetLayout(device(), m_scene_set_layout, nullptr);
    vkDestroyDescriptorSetLayout(device(), m_view_set_layout, nullptr);

    disconnect_registry_callbacks();
}

void ObjectRenderer::render(const RenderArguments& args) {
    RenderState rs = {
        .cmd           = *args.subpass_cmd,
        .compute_cmd   = *args.compute_cmd,
        .render_target = &m_render_targets.at(args.render_target_name),
    };

    update_view_set(rs.render_target);
    update_lights();

    render_direct(rs);
}

void ObjectRenderer::render_direct(RenderState& rs) {
    auto view = m_registry->view<Renderable, Transform>();

    rs.cmd.bind_descriptor_set(SCENE_SET, get_framely().scene_set);
    rs.cmd.bind_descriptor_set(VIEW_SET, rs.render_target->view_sets[m_render_server->get_frame_index()]);

    auto transform_view = m_registry->view<Transform>();
    auto parent_view    = m_registry->view<CParent>();

    auto get_model_matrix = vke::make_y_combinator([&](auto&& self, entt::entity e) -> glm::mat4 {
        glm::mat4 model_mat = transform_view->get(e).local_model_matrix();

        if (parent_view.contains(e)) {
            auto parent = parent_view->get(e).parent;

            model_mat = self(parent) * model_mat;
        }

        return model_mat;
    });

    for (auto& e : view) {
        auto [r, t] = view.get(e);

        glm::mat4 model_mat = get_model_matrix(e);

        auto* model = m_resource_manager->get_model(r.model_id);

        if (model == nullptr) {
            continue;
        }

        struct Push {
            glm::mat4 model_matrix;
            glm::mat4 normal_matrix;
        };

        Push push{
            .model_matrix  = model_mat,
            .normal_matrix = glm::transpose(glm::inverse(glm::mat3(model_mat))),
            // .normal_matrix = model_mat,
        };

        for (auto& part : model->parts) {
            if (!m_resource_manager->bind_material(rs, part.material_id)) continue;
            if (!m_resource_manager->bind_mesh(rs, part.mesh_id)) continue;

            auto* mesh = rs.mesh;

            rs.cmd.push_constant(&push);
            rs.cmd.draw_indexed(mesh->index_count, 1, 0, 0, 0);
        }
    }
}

void ObjectRenderer::create_render_target(const std::string& name, const std::string& subpass_name,bool allow_indirect_render) {
    RenderTarget target = {
        .renderpass_name = subpass_name,
        .subpass_type    = m_resource_manager->get_subpass_type(subpass_name),
        .camera          = nullptr,
    };

    for (int i = 0; i < FRAME_OVERLAP; i++) {
        target.view_buffers[i] = std::make_unique<vke::Buffer>(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, sizeof(ViewData), true);

        vke::DescriptorSetBuilder builder;
        builder.add_ubo(target.view_buffers[i].get(), VK_SHADER_STAGE_ALL);

        if (target.buffers) {
            auto* render_buffers = target.buffers.get();

            builder.add_ssbo(render_buffers->instance_draw_parameter_location_buffer[i].get(), VK_SHADER_STAGE_COMPUTE_BIT); // instance_draw_parameter_locations
            builder.add_ssbo(render_buffers->instance_count_buffer.get(), VK_SHADER_STAGE_COMPUTE_BIT);                      // instance_counters
            builder.add_ssbo(render_buffers->part2indirect_draw_location[i].get(), VK_SHADER_STAGE_COMPUTE_BIT);             // indirect_draw_locations
            builder.add_ssbo(render_buffers->indirect_draw_buffer.get(), VK_SHADER_STAGE_COMPUTE_BIT);                       // draw_commands
            builder.add_ssbo(render_buffers->instance_draw_parameters.get(), VK_SHADER_STAGE_ALL);                           // instance_draw_parameters
        } else {
            builder.add_ssbo(m_dummy_buffer.get(), VK_SHADER_STAGE_COMPUTE_BIT); // instance_draw_parameter_locations
            builder.add_ssbo(m_dummy_buffer.get(), VK_SHADER_STAGE_COMPUTE_BIT); // instance_counters
            builder.add_ssbo(m_dummy_buffer.get(), VK_SHADER_STAGE_COMPUTE_BIT); // indirect_draw_locations
            builder.add_ssbo(m_dummy_buffer.get(), VK_SHADER_STAGE_COMPUTE_BIT); // draw_commands
            builder.add_ssbo(m_dummy_buffer.get(), VK_SHADER_STAGE_ALL);         // instance_draw_parameters
        }

        target.view_sets[i] = builder.build(m_descriptor_pool.get(), m_view_set_layout);
    }

    m_render_targets[name] = std::move(target);
}

void ObjectRenderer::update_lights() {
    auto view         = m_registry->view<Transform, CPointLight>();
    auto point_lights = vke::map_vec(view, [&](const entt::entity& e) {
        auto [t, light] = view.get(e);
        return PointLight{
            .color = glm::vec4(light.color, 0.0),
            .pos   = t.position,
            .range = light.range,
        };
    });

    if (point_lights.size() > MAX_LIGHTS) {
        point_lights.resize(MAX_LIGHTS);
    }

    auto light_buffer = get_framely().light_buffer.get();
    auto& lights      = light_buffer->mapped_data<SceneLightData>()[0];

    lights.point_light_count = point_lights.size();
    for (int i = 0; i < point_lights.size(); i++) {
        lights.point_lights[i] = point_lights[i];
    }

    lights.ambient_light = glm::vec4(0.1, 0.2, 0.2, 0.0);

    lights.directional_light.dir   = glm::vec4(glm::normalize(glm::vec3(0.2, 1.0, 0.1)), 0.0);
    lights.directional_light.color = glm::vec4(0.9);
}

void ObjectRenderer::update_view_set(RenderTarget* target) {
    assert(target->camera);

    auto* ubo  = target->view_buffers[m_render_server->get_frame_index()].get();
    auto& data = ubo->mapped_data<ViewData>()[0];

    data.proj_view      = target->camera->proj_view();
    data.inv_proj_view  = glm::inverse(data.proj_view);
    data.view_world_pos = dvec4(target->camera->world_position, 0.0);
}

ObjectRenderer::FramelyData& ObjectRenderer::get_framely() {
    return m_framely_data[m_render_server->get_frame_index()];
}
void ObjectRenderer::create_default_pbr_pipeline() {
    std::string pipelines[] = {"vke::default"};
    m_resource_manager->create_multi_target_pipeline(pbr_pipeline_name, pipelines);
}

void on_renderable_construct(entt::registry& r, entt::entity e) {

    printf("entity %d has been attached renderable component\n", e);
}

void ObjectRenderer::set_entt_registry(entt::registry* registry) {
    if (m_registry != nullptr) {
        // handle registry change
        TODO();
    }
    m_registry = registry;

    connect_registry_callbacks();
}

void ObjectRenderer::renderable_component_creation_callback(entt::registry&, entt::entity e) {
    m_scene_data->pending_entities_for_register.push_back(e);
}

void ObjectRenderer::renderable_component_update_callback(entt::registry&, entt::entity e) {
    printf("entity %d's renderable component has been modified\n", e);
    TODO();
}

void ObjectRenderer::renderable_component_destroy_callback(entt::registry& r, entt::entity e) {
    auto instance = r.try_get<InstanceComponent>(e);
    if (instance == nullptr) return;

    m_scene_data->pending_instances_for_destruction.push_back(instance->id);
}

void ObjectRenderer::connect_registry_callbacks() {
    if (!m_scene_data) return;

    m_registry->on_construct<Renderable>().connect<&ObjectRenderer::renderable_component_creation_callback>(this);
    m_registry->on_update<Renderable>().connect<&ObjectRenderer::renderable_component_update_callback>(this);
    m_registry->on_destroy<Renderable>().connect<&ObjectRenderer::renderable_component_destroy_callback>(this);
}

void ObjectRenderer::disconnect_registry_callbacks() {
    m_registry->on_construct<Renderable>().disconnect(this);
    m_registry->on_update<Renderable>().disconnect(this);
    m_registry->on_destroy<Renderable>().disconnect(this);
}

void ObjectRenderer::updates_for_indirect_render(vke::CommandBuffer& cmd) {
    auto& resource_updates = m_resource_manager->get_updated_resource();

    StencilBuffer stencil;

    for (auto model_id : resource_updates.model_updates) {
        auto* model = m_resource_manager->get_model(model_id);

        auto allocation                                    = m_scene_data->model_part_buffer_sub_allocator.allocate(model->parts.size()).value();
        m_scene_data->model_part_sub_allocations[model_id] = allocation;

        ModelData model_data = {
            .part_index = allocation.offset,
            .part_count = allocation.size,
        };

        stencil.copy_data(m_scene_data->model_info_buffer->subspan_item<ModelData>(model_id.id, 1), &model_data, 1);

        auto parts = map_vec2small_vec(model->parts, [&](const ResourceManager::RenderModel::Part& part) {
            return PartData{
                .mesh_id     = part.mesh_id.id,
                .material_id = part.material_id.id,
            };
        });

        stencil.copy_data(m_scene_data->model_part_info_buffer->subspan_item<PartData>(allocation.offset, allocation.size), parts.as_const_span());
    }

    // for (auto material_id : resource_updates.material_updates) {
    //     auto* material = m_resource_manager->get_material(material_id);
    // }

    for (auto mesh_id : resource_updates.mesh_updates) {
        auto* mesh = m_resource_manager->get_mesh(mesh_id);

        MeshData mesh_data = {
            .index_offset = 0,
            .index_count  = mesh->index_count,
        };

        stencil.copy_data(m_scene_data->mesh_info_buffer->subspan_item<MeshData>(mesh_id.id, 1), &mesh_data, 1);
    }

    flush_pending_entities(cmd, stencil);

    stencil.flush_copies(cmd);
}

static glm::vec4 quat2vec4(const glm::quat& q) { return glm::vec4(q.x, q.y, q.z, q.w); }

void ObjectRenderer::flush_pending_entities(vke::CommandBuffer& cmd, StencilBuffer& stencil) {
    auto renderable_view         = m_registry->view<Renderable, Transform>();
    auto instance_component_view = m_registry->view<InstanceComponent>();

    for (auto entity : m_scene_data->pending_entities_for_register) {
        auto [model, transform] = renderable_view.get(entity);
        auto model_id           = model.model_id;

        InstanceData instance_data = {
            .world_position = glm::dvec4(transform.position, 0.0),
            .rotation       = glm::vec4(quat2vec4(transform.rotation)),
            .model_id       = model_id.id,
        };

        auto instance_id = m_scene_data->instance_id_manager.new_id();
        stencil.copy_data(m_scene_data->instance_buffer->subspan_item<InstanceData>(instance_id.id, 1), &instance_data, 1);
    }
}

void ObjectRenderer::render_indirect(RenderState& state) {
    updates_for_indirect_render(state.compute_cmd);

    struct PartEntry {
        u32 part_id;
        MeshID mesh_id;
    };

    std::unordered_map<MaterialID, SmallVec<PartEntry>> material_part_ids;

    auto* draw_data = state.render_target->buffers.get();

    u32 total_instance_counter   = 0;
    auto allocate_instance_space = [&](u32 instance_count) {
        u32 index = total_instance_counter;
        total_instance_counter += instance_count;
        return glm::uvec2(index, instance_count);
    };

    auto instance_offsets = draw_data->instance_draw_parameter_location_buffer[m_render_server->get_frame_index()]->mapped_data_as_span<glm::uvec2>();

    for (auto& [model_id, instance_count] : m_scene_data->model_instance_counters) {
        assert(instance_count < 0);
        if (instance_count <= 0) continue;

        auto* model       = m_resource_manager->get_model(model_id);
        auto& model_parts = m_scene_data->model_part_sub_allocations[model_id];

        for (u32 i = 0; i < model->parts.size(); i++) {
            auto& part = model->parts[i];

            u32 part_id = model_parts.offset + i;

            material_part_ids[part.material_id].push_back({
                .part_id = part_id,
                .mesh_id = part.mesh_id,
            });

            instance_offsets[part_id] = allocate_instance_space(instance_count);
        }
    }

    state.cmd.bind_descriptor_set(SCENE_SET, get_framely().scene_set);
    state.cmd.bind_descriptor_set(VIEW_SET, state.render_target->view_sets[m_render_server->get_frame_index()]);

    auto indirect_draw_location = draw_data->part2indirect_draw_location[m_render_server->get_frame_index()]->mapped_data<u32>();
    auto indirect_draw_buffer   = draw_data->indirect_draw_buffer.get();

    u32 total_indirect_draws = 0;
    for (auto& [material_id, parts] : material_part_ids) {
        m_resource_manager->bind_material(state, material_id);

        for (u32 i = 0; i < parts.size(); i++) {
            auto& part              = parts[i];
            u32 part_id             = part.part_id;
            u32 indirect_draw_index = total_indirect_draws + i;

            m_resource_manager->bind_mesh(state, part.mesh_id);

            indirect_draw_location[part_id] = indirect_draw_index;

            state.cmd.draw_indexed_indirect(indirect_draw_buffer->subspan_item<VkDrawIndexedIndirectCommand>(indirect_draw_index, 1), 1);
        }

        total_indirect_draws += parts.size();
    }

    state.compute_cmd.bind_descriptor_set(SCENE_SET, get_framely().scene_set);
    state.compute_cmd.bind_descriptor_set(VIEW_SET, state.render_target->view_sets[m_render_server->get_frame_index()]);

    state.compute_cmd.bind_pipeline(m_scene_data->cull_pipeline.get());
    state.compute_cmd.dispatch(1, 1, 1);

    VkBufferMemoryBarrier buffer_barriers[] = {
        VkBufferMemoryBarrier{
            .sType         = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
            .srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT,
            .dstAccessMask = VK_ACCESS_MEMORY_READ_BIT,
            .buffer        = draw_data->instance_count_buffer->handle(),
            .offset        = 0,
            .size          = VK_WHOLE_SIZE,
        },
    };

    state.compute_cmd.pipeline_barrier({
        .src_stage_mask         = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
        .dst_stage_mask         = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
        .buffer_memory_barriers = buffer_barriers,
    });

    state.compute_cmd.bind_pipeline(m_scene_data->indirect_draw_command_gen_pipeline.get());
    state.compute_cmd.dispatch(1, 1, 1);

    VkBufferMemoryBarrier buffer_barriers2[] = {
        VkBufferMemoryBarrier{
            .sType         = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
            .srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT,
            .dstAccessMask = VK_ACCESS_MEMORY_READ_BIT,
            .buffer        = draw_data->instance_draw_parameters->handle(),
            .offset        = 0,
            .size          = VK_WHOLE_SIZE,
        },
        VkBufferMemoryBarrier{
            .sType         = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
            .srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT,
            .dstAccessMask = VK_ACCESS_INDIRECT_COMMAND_READ_BIT,
            .buffer        = draw_data->indirect_draw_buffer->handle(),
            .offset        = 0,
            .size          = VK_WHOLE_SIZE,
        },
    };

    state.compute_cmd.pipeline_barrier({
        .src_stage_mask         = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
        .dst_stage_mask         = VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT | VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_MESH_SHADER_BIT_EXT,
        .buffer_memory_barriers = buffer_barriers2,
    });
}

} // namespace vke
