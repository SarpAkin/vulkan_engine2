#include "scene_set.hpp"

#include <vke/pipeline_loader.hpp>
#include <vke/vke.hpp>
#include <vke/vke_builders.hpp>

#include "render/mesh/shader/scene_data.h"

#include "render/render_server.hpp"
#include "scene/camera.hpp"

namespace vke {

SceneSet::SceneSet(RenderServer* render_server) {
    m_render_server = render_server;
    vke::DescriptorSetLayoutBuilder builder;


    builder.add_binding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL, 1);

    m_descriptor_set_layout = builder.build();

    render_server->get_pipeline_loader()->get_pipeline_globals_provider()->set_layouts["vke::scene_set"] = m_descriptor_set_layout;

    for (auto& data : m_framely_datas) {
        data.scene_ubo = std::make_unique<Buffer>(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, sizeof(ViewData), true);

        vke::DescriptorSetBuilder set_builder;
        set_builder.add_ubo(data.scene_ubo.get(), VK_SHADER_STAGE_ALL);
        data.set = set_builder.build(render_server->get_descriptor_pool(), m_descriptor_set_layout);
    }
}

SceneSet::~SceneSet() {}

VkDescriptorSet SceneSet::get_scene_set() const { return m_framely_datas[m_render_server->get_frame_index()].set; }

void SceneSet::update_scene_set() {
    auto* ubo = m_framely_datas[m_render_server->get_frame_index()].scene_ubo.get();

    auto& data = ubo->mapped_data<ViewData>()[0];

    data.proj_view             = camera->proj_view();
    data.inv_proj_view         = glm::inverse(data.proj_view);
    data.view_world_pos        = dvec4(camera->world_position, 0.0);
}
} // namespace vke