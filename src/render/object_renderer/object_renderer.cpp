#include "object_renderer.hpp"

#include "render/mesh/shader/scene_data.h"
#include "render/render_server.hpp"
#include "scene/camera.hpp"
#include "scene/components/components.hpp"
#include "scene/components/transform.hpp"
#include "scene/scene.hpp"

#include "render_state.hpp"

#include <vke/pipeline_loader.hpp>
#include <vke/vke_builders.hpp>

namespace vke {

ObjectRenderer::ObjectRenderer(RenderServer* render_server) {
    m_render_server   = render_server;
    m_descriptor_pool = std::make_unique<DescriptorPool>();

    {
        vke::DescriptorSetLayoutBuilder layout_builder;
        layout_builder.add_ssbo(VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT, 1);
        m_scene_set_layout = layout_builder.build();
    }

    {
        vke::DescriptorSetLayoutBuilder layout_builder;
        layout_builder.add_ubo(VK_SHADER_STAGE_ALL, 1);
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
        framely.scene_set = builder.build(m_descriptor_pool.get(), m_scene_set_layout);
    }
}

ObjectRenderer::~ObjectRenderer() {
    vkDestroyDescriptorSetLayout(device(), m_scene_set_layout, nullptr);
    vkDestroyDescriptorSetLayout(device(), m_view_set_layout, nullptr);
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

void ObjectRenderer::render_indirect(RenderState& state) {
    


}

void ObjectRenderer::create_render_target(const std::string& name, const std::string& subpass_name) {
    RenderTarget target = {
        .renderpass_name = subpass_name,
        .subpass_type    = m_resource_manager->get_subpass_type(subpass_name),
        .camera          = nullptr,
    };

    for (int i = 0; i < FRAME_OVERLAP; i++) {
        target.view_buffers[i] = std::make_unique<vke::Buffer>(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, sizeof(ViewData), true);

        vke::DescriptorSetBuilder builder;
        builder.add_ubo(target.view_buffers[i].get(), VK_SHADER_STAGE_ALL);
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
} // namespace vke
