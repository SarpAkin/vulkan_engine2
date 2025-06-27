#include "object_renderer.hpp"

#include "imgui.h"
#include "render/object_renderer/light_buffers_manager.hpp"
#include "render/object_renderer/sub_systems/indirect_model_renderer.hpp"
#include "render/render_server.hpp"
#include "render/shader/scene_data.h"
#include "scene/camera.hpp"
#include "scene/components/components.hpp"
#include "scene/components/transform.hpp"
#include "scene/scene.hpp"
#include "scene_buffers_manager.hpp"

#include "render/debug/gpu_timing_system.hpp"

#include "render_util.hpp"

#include "render_state.hpp"

#include "hierarchical_z_buffers.hpp"

#include <format>
#include <vke/pipeline_loader.hpp>
#include <vke/util.hpp>
#include <vke/vke.hpp>
#include <vke/vke_builders.hpp>

#include <vk_mem_alloc.h>

namespace vke {

ObjectRenderer::ObjectRenderer(RenderServer* render_server) {
    m_render_server   = render_server;
    m_descriptor_pool = std::make_unique<DescriptorPool>();

    m_dummy_buffer = std::make_unique<vke::Buffer>(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, 256, false);

    // View set layout
    {
        vke::DescriptorSetLayoutBuilder layout_builder;
        layout_builder.add_ubo(VK_SHADER_STAGE_ALL); // scene_view

        layout_builder.add_image_sampler(VK_SHADER_STAGE_COMPUTE_BIT);

        m_view_set_layout = layout_builder.build();
    }

    auto pg_provider = m_render_server->get_pipeline_loader()->get_pipeline_globals_provider();

    m_resource_manager = std::make_unique<ResourceManager>(render_server);

    pg_provider->set_layouts["vke::object_renderer::material_set"] = m_resource_manager->get_material_set_layout();
    pg_provider->set_layouts["vke::object_renderer::view_set"]     = m_view_set_layout;


    for (auto& framely : m_framely_data) {
    }

    create_render_systems();
}

ObjectRenderer::~ObjectRenderer() {
    vkDestroyDescriptorSetLayout(device(), m_view_set_layout, nullptr);
}

static bool indirect_render_enabled = true;
void ObjectRenderer::update_scene_data(CommandBuffer& cmd) {
    m_light_manager->flush_pending_lights(cmd);

    for(auto& rs : m_render_systems){
        rs->update(cmd);
    }
}

void ObjectRenderer::render(const RenderArguments& args) {
    auto* rd = &m_render_targets.at(args.render_target_name);
    update_view_set(rd);

    auto args_copy = args;

    for(auto& rs : m_render_systems){
        rs->render(args_copy);
    }
}

void ObjectRenderer::create_render_target(const std::string& name, const std::string& subpass_name, const RenderTargetArguments& render_target_arguments) {
    RenderTarget target = {
        .info = RenderTargetInfo{
            .subpass_name = subpass_name,
            .camera       = nullptr,
        }};

    for (int i = 0; i < FRAME_OVERLAP; i++) {
        target.view_buffers[i] = std::make_unique<vke::Buffer>(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, sizeof(ViewData), true);

        update_view_descriptor_set(&target, i);
    }

    m_render_targets[name] = std::move(target);

    for(auto& rs : m_render_systems){
        rs->register_render_target(name);
    }
}

void ObjectRenderer::update_view_descriptor_set(RenderTarget* target, u32 i) {
    vke::DescriptorSetBuilder builder;
    builder.add_ubo(target->view_buffers[i].get(), VK_SHADER_STAGE_ALL);

    auto* hzb      = target->hzb;
    auto hzb_stage = VK_SHADER_STAGE_COMPUTE_BIT;
    if (hzb) {
        builder.add_image_sampler(hzb->get_mips(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, hzb->get_sampler(), hzb_stage);
    } else {
        builder.add_image_sampler(m_resource_manager->get_null_texture(), VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL_KHR, m_resource_manager->get_nearest_sampler(), hzb_stage);
    }

    target->info.view_sets[i] = builder.build(m_descriptor_pool.get(), m_view_set_layout);
}

void ObjectRenderer::update_view_set(RenderTarget* target) {
    assert(target->info.camera);

    auto frame_index = m_render_server->get_frame_index();

    auto* ubo  = target->view_buffers[m_render_server->get_frame_index()].get();
    auto& data = ubo->mapped_data<ViewData>()[0];

    auto proj_view = target->info.camera->proj_view();

    data.proj_view      = proj_view;
    data.inv_proj_view  = glm::inverse(proj_view);
    data.view_world_pos = glm::dvec4(target->info.camera->get_world_pos(), 0.0);

    data.frustum = calculate_frustum(data.inv_proj_view);

    if (target->is_view_set_needs_update[frame_index]) {
        update_view_descriptor_set(target, frame_index);

        target->is_view_set_needs_update[frame_index] = 0;
    }

    if (target->hzb) {
        data.old_proj_view            = target->hzb->get_hzb_proj_view();
        data.is_hzb_culling_enabled.x = 1;
    } else {
        data.is_hzb_culling_enabled.x = 0;
    }
}

ObjectRenderer::FramelyData& ObjectRenderer::get_framely() {
    return m_framely_data[m_render_server->get_frame_index()];
}
void ObjectRenderer::create_default_pbr_pipeline() {
}


void ObjectRenderer::set_world(flecs::world* world) {
    if (m_world != nullptr) {
        // handle registry change
        TODO();
    }
    m_world = world;

    for(auto& rs : m_render_systems){
        rs->set_world(m_world);
    }

    m_light_manager = std::make_unique<LightBuffersManager>(m_render_server, world);
}


IBuffer* ObjectRenderer::get_view_buffer(const std::string& render_target_name, int frame_index) const {
    assert(frame_index >= 0 && frame_index < FRAME_OVERLAP);

    return m_render_targets.at(render_target_name).view_buffers[frame_index].get();
}

void ObjectRenderer::set_hzb(const std::string& render_target, HierarchicalZBuffers* hzb) {
    auto* rd = &m_render_targets.at(render_target);
    rd->hzb  = hzb;

    for (bool& b : rd->is_view_set_needs_update) {
        b = true;
    }
}

void ObjectRenderer::set_camera(const std::string& render_target, Camera* camera) { m_render_targets.at(render_target).info.camera = camera; }

void ObjectRenderer::create_render_systems() {
    add_render_system(std::make_unique<vke::IndirectModelRenderer>(this));
}
} // namespace vke
