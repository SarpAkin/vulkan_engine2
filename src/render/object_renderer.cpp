#include "object_renderer.hpp"

#include "render/mesh/shader/scene_data.h"
#include "render_server.hpp"
#include "scene/camera.hpp"
#include "scene/components/components.hpp"
#include "scene/components/transform.hpp"
#include "scene/scene.hpp"
#include "scene_set.hpp"

#include <vke/pipeline_loader.hpp>
#include <vke/vke_builders.hpp>

namespace vke {

ObjectRenderer::ObjectRenderer(RenderServer* render_server) {
    m_render_server   = render_server;
    m_descriptor_pool = std::make_unique<DescriptorPool>();

    {
        vke::DescriptorSetLayoutBuilder layout_builder;
        layout_builder.add_image_sampler(VK_SHADER_STAGE_FRAGMENT_BIT, 4);
        // layout_builder.add_ubo(VK_SHADER_STAGE_ALL, 1);
        m_material_set_layout = layout_builder.build();
    }

    {
        vke::DescriptorSetLayoutBuilder layout_builder;
        layout_builder.add_ssbo(VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT, 1);
        m_scene_light_set_layout = layout_builder.build();
    }

    VkSamplerCreateInfo sampler_info{
        .sType        = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
        .magFilter    = VK_FILTER_NEAREST,
        .minFilter    = VK_FILTER_NEAREST,
        .addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT,
        .addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT,
        .addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT,
    };

    VK_CHECK(vkCreateSampler(device(), &sampler_info, nullptr, &m_nearest_sampler));

    create_null_texture(16);

    auto pg_provider = m_render_server->get_pipeline_loader()->get_pipeline_globals_provider();

    pg_provider->set_layouts["vke::object_renderer::material_set"] = m_material_set_layout;
    pg_provider->set_layouts["vke::object_renderer::light_set"]    = m_scene_light_set_layout;

    m_scene_set = std::make_unique<SceneSet>(render_server);

    for (auto& framely : m_framely_data) {
        framely.light_buffer = std::make_unique<Buffer>(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, sizeof(SceneLightData), true);
        vke::DescriptorSetBuilder builder;
        builder.add_ssbo(framely.light_buffer.get(), VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT);
        framely.scene_light_set = builder.build(m_descriptor_pool.get(), m_scene_light_set_layout);
    }
}

ObjectRenderer::~ObjectRenderer() {
    vkDestroyDescriptorSetLayout(device(), m_material_set_layout, nullptr);
    vkDestroySampler(device(), m_nearest_sampler, nullptr);
}

void ObjectRenderer::render(vke::CommandBuffer& cmd) {
    auto view = m_registry->view<Renderable, Transform>();

    RenderState state = {.cmd = cmd};

    m_scene_set->update_scene_set();
    update_lights();

    cmd.bind_descriptor_set(SCENE_SET, m_scene_set->get_scene_set());
    cmd.bind_descriptor_set(LIGHT_SET, get_framely().scene_light_set);

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

        auto& model = m_render_models[r.model_id];

        struct Push {
            glm::mat4 model_matrix;
            glm::mat4 normal_matrix;
        };

        Push push{
            .model_matrix  = model_mat,
            .normal_matrix = glm::transpose(glm::inverse(glm::mat3(model_mat))),
            // .normal_matrix = model_mat,
        };

        for (auto& part : model.parts) {
            if (!bind_material(state, part.material_id)) continue;
            if (!bind_mesh(state, part.mesh_id)) continue;

            auto* mesh = state.mesh;

            cmd.push_constant(&push);
            cmd.draw_indexed(mesh->index_count, 1, 0, 0, 0);
        }
    }
}

bool ObjectRenderer::bind_mesh(RenderState& state, MeshID id) {
    if (state.bound_mesh_id == id) return true;

    if (auto it = m_meshes.find(id); it != m_meshes.end()) {
        state.mesh = &it->second;
    } else {
        LOG_ERROR("failed to bind mesh with id %d", id.id);
        return false;
    }

    state.cmd.bind_index_buffer(state.mesh->index_buffer.get(), state.mesh->index_type);
    auto vbs = map_vec2small_vec<6>(state.mesh->vertex_buffers, [&](const auto& b) -> const IBufferSpan* { return b.get(); });
    state.cmd.bind_vertex_buffer(std::span(vbs));

    return true;
}

bool ObjectRenderer::bind_material(RenderState& state, MaterialID id) {
    if (state.bound_material_id == id) return true;

    if (auto it = m_materials.find(id); it != m_materials.end()) {
        state.material = &it->second;
    } else {
        LOG_ERROR("failed to bind material with id %d", id.id);
        return false;
    }

    if (state.bound_pipeline != state.material->pipeline.get()) {
        state.bound_pipeline = state.material->pipeline.get();
        state.cmd.bind_pipeline(state.bound_pipeline);
    }
    state.cmd.bind_descriptor_set(MATERIAL_SET, state.material->material_set);

    return true;
}

MaterialID ObjectRenderer::create_material(const std::string& pipeline_name, std::vector<ImageID> images, const std::string& material_name) {
    images.resize(4, m_null_texture_id);

    Material m{
        .pipeline     = load_pipeline_cached(pipeline_name),
        .material_set = VK_NULL_HANDLE,
        .images       = images,
        .name         = material_name,
    };

    auto views = vke::map_vec(images, [&](auto& id) {
        return std::pair(get_image(id), m_nearest_sampler);
    });

    vke::DescriptorSetBuilder builder;
    builder.add_image_samplers(views, VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL, VK_SHADER_STAGE_FRAGMENT_BIT);
    m.material_set = builder.build(m_descriptor_pool.get(), m_material_set_layout);

    auto id = MaterialID(new_raw_id());

    if (!material_name.empty()) {
        assert(!m_material_names2material_ids.contains(material_name) && "material name is already present");
        m_material_names2material_ids[material_name] = id;
    }

    m_materials[id] = std::move(m);
    return id;
}

MeshID ObjectRenderer::create_mesh(Mesh mesh, const std::string& name) {
    auto id = MeshID(new_raw_id());

    m_meshes[id] = std::move(mesh);

    if (!name.empty()) {
        assert(!m_mesh_names2mesh_ids.contains(name) && "mesh name is already present");
        m_mesh_names2mesh_ids[name] = id;
    }

    return id;
}

RenderModelID ObjectRenderer::create_model(MeshID mesh, MaterialID material, const std::string& name) {
    auto id = RenderModelID(new_raw_id());

    m_render_models[id] = RenderModel{
        .parts = {
            {mesh, material},
        },
    };

    if (!name.empty()) {
        bind_name2model(id, name);
    }

    return id;
}

RenderModelID ObjectRenderer::create_model(const std::vector<std::pair<MeshID, MaterialID>>& parts, const std::string& name) {
    auto id = RenderModelID(new_raw_id());

    m_render_models[id] = RenderModel{
        .parts = map_vec(parts, [](auto& part) {
        auto& [mesh, mat] = part;
        return RenderModel::Part{mesh, mat};
    }),
    };

    if (!name.empty()) {
        bind_name2model(id, name);
    }

    return id;
}

void ObjectRenderer::bind_name2model(RenderModelID id, const std::string& name) {
    assert(!m_render_model_names2model_ids.contains(name) && "model name is already present");

    m_render_model_names2model_ids[name] = id;
    m_render_models[id].name             = name;
}

ImageID ObjectRenderer::create_image(std::unique_ptr<IImageView> image_view, const std::string& name) {
    auto id = ImageID(new_raw_id());

    m_images[id] = std::move(image_view);

    if (!name.empty()) {
        m_image_names2image_ids[name] = id;
    }

    return id;
}

void ObjectRenderer::create_null_texture(int size) {
    auto image = std::make_unique<vke::Image>(ImageArgs{
        .format       = VK_FORMAT_R8G8B8A8_SRGB,
        .usage_flags  = VK_IMAGE_USAGE_SAMPLED_BIT,
        .width        = static_cast<u32>(size),
        .height       = static_cast<u32>(size),
        .layers       = 1,
        .host_visible = true,
    });

    auto pixels   = image->mapped_data_ptr<u32>();
    int half_size = size / 2;

    u32 BLACK  = 0xFF'00'00'00;
    u32 PURPLE = 0xFF'BB'00'BB;

    for (int y = 0; y < size; y++) {
        for (int x = 0; x < size; x++) {
            pixels[x + y * size] = (y > half_size) ^ (x > half_size) ? BLACK : PURPLE;
        }
    }

    m_null_texture    = image.get();
    m_null_texture_id = create_image(std::move(image), "null");

    VulkanContext::get_context()->immediate_submit([&](CommandBuffer& cmd) {
        VkImageMemoryBarrier barriers[] = {
            VkImageMemoryBarrier{
                .sType            = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
                .srcAccessMask    = VK_ACCESS_HOST_WRITE_BIT,
                .dstAccessMask    = VK_ACCESS_SHADER_READ_BIT,
                .oldLayout        = VK_IMAGE_LAYOUT_UNDEFINED,
                .newLayout        = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                .image            = m_null_texture->vke_image()->handle(),
                .subresourceRange = {
                    .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                    .levelCount = 1,
                    .layerCount = 1,
                }},
        };

        cmd.pipeline_barrier(PipelineBarrierArgs{
            .src_stage_mask        = VK_PIPELINE_STAGE_HOST_BIT,
            .dst_stage_mask        = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            .image_memory_barriers = barriers,
        });
    });
}

IImageView* ObjectRenderer::get_image(ImageID id) {
    auto it = m_images.find(id);
    if (it != m_images.end()) {
        return it->second.get();
    } else {
        return m_null_texture;
    }
}

RCResource<vke::IPipeline> ObjectRenderer::load_pipeline_cached(const std::string& name) {
    auto val = vke::at(m_cached_pipelines, name);
    if (val.has_value()) return val.value();

    RCResource<IPipeline> pipeline = m_render_server->get_pipeline_loader()->load(name.c_str());
    m_cached_pipelines[name]       = pipeline;

    return pipeline;
}

void ObjectRenderer::set_scene(Scene* scene) {
    m_scene_set->scene  = scene;
    m_scene_set->camera = scene->get_camera();
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

ObjectRenderer::FramelyData& ObjectRenderer::get_framely() {
    return m_framely_data[m_render_server->get_frame_index()];
}
} // namespace vke
