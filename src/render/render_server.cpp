#include "render_server.hpp"

#include "mesh/mesh_util.hpp"
#include "mesh/shapes.hpp"
#include "render/imgui/imgui_manager.hpp"
#include "render/object_renderer/object_renderer.hpp"
#include "render/object_renderer/resource_manager.hpp"
#include "scene/camera.hpp"
#include "scene/components/transform.hpp"
#include "window/window_sdl.hpp"

#include <vke/pipeline_loader.hpp>
#include <vke/util.hpp>
#include <vke/vke.hpp>

#include <chrono>

namespace vke {

void RenderServer::init() {
    vke::ContextConfig config{
        .app_name    = "app0",
        .features1_0 = {
            .shaderFloat64 = true,
            .shaderInt64   = true,
            .shaderInt16   = true,
            .sparseBinding = true,
            .sparseResidencyBuffer = true,
        },
        .features1_2 = {
            .shaderInt8 = true,
        },
    };

    vke::VulkanContext::init(config);

    m_descriptor_pool = std::make_unique<DescriptorPool>();

    m_window = std::make_unique<WindowSDL>();
    m_window->init_surface();

    m_window_renderpass = vke::make_simple_windowed_renderpass(m_window.get(), true);

    m_imgui_manager = std::make_unique<ImguiManager>(m_window.get(), m_window_renderpass.get(), 0);
    dynamic_cast<WindowSDL*>(m_window.get())->set_imgui_manager(m_imgui_manager.get());

    m_pipeline_loader = vke::IPipelineLoader::make_debug_loader("src/");

    auto pg_provider = std::make_unique<vke::PipelineGlobalsProvider>();
    pg_provider->subpasses.emplace("vke::default_forward", std::make_unique<SubpassDetails>(*m_window_renderpass->get_subpass(0)));
    pg_provider->vertex_input_descriptions.emplace("vke::default_mesh", vke::make_default_vertex_layout());

    m_pipeline_loader->set_pipeline_globals_provider(std::move(pg_provider));

    for (int i = 0; i < FRAME_OVERLAP; i++) {
        auto _pool = std::make_unique<vke::CommandPool>();
        auto* pool = _pool.get();
        m_framely_data.push_back({
            .cmd_pool = std::move(_pool),
            .cmd   = pool->allocate(),
            .main_pass_cmd = pool->allocate(false),
            .fence = std::make_unique<vke::Fence>(true),
        });
    }

    m_object_renderer = std::make_unique<ObjectRenderer>(this);

    m_object_renderer->get_resource_manager()->set_subpass_type("vke::default_forward", MaterialSubpassType::FORWARD);
    m_object_renderer->create_default_pbr_pipeline();
    // Creating a cube model
    //  auto materialID = m_object_renderer->create_material("vke::default", {}, "vke::default_material");
    //  auto meshID     = m_object_renderer->create_mesh(std::move(*vke::make_cube()));
    //  m_object_renderer->create_model(meshID, materialID,"cube");
}

void RenderServer::frame(std::function<void(FrameArgs& args)> render_function) {
    m_window->poll_events();

    auto& framely_data = m_framely_data[m_frame_index];

    VkFence fence = framely_data.fence->handle();
    VK_CHECK(vkWaitForFences(device(), 1, &fence, true, 1E9));

    if (!m_window->surface()->prepare(1E9)) {
        VK_CHECK(vkDeviceWaitIdle(device()));
        m_window->surface()->recrate_swapchain();
        LOG_WARNING("failed to prepare window surface. skipping rendering");
        return;
    }

    m_imgui_manager->new_frame();
    VK_CHECK(vkResetFences(device(), 1, &fence));


    auto& main_renderpass_pass_cmd = *framely_data.main_pass_cmd;
    main_renderpass_pass_cmd.reset();
    main_renderpass_pass_cmd.begin_secondary(m_window_renderpass->get_subpass(0));

    auto& top_cmd = *framely_data.cmd;
    top_cmd.reset();
    top_cmd.begin();

    auto args = FrameArgs{
        .main_pass_cmd        = &main_renderpass_pass_cmd,
        .primary_cmd = &top_cmd,
        // .pre_pass_compute_cmd = &pre_pass_comput_cmd,
    };

    render_function(args);

    m_imgui_manager->flush_frame(main_renderpass_pass_cmd);

    main_renderpass_pass_cmd.end();

    m_window_renderpass->set_external(true);
    m_window_renderpass->begin(top_cmd);

    top_cmd.execute_secondaries(&main_renderpass_pass_cmd);

    m_window_renderpass->end(top_cmd);

    top_cmd.end();

    std::vector<VkSemaphore> prepare_semaphores      = {m_window->surface()->get_prepare_semaphore()->handle()};
    std::vector<VkSemaphore> present_wait_semaphores = {m_window->surface()->get_wait_semaphore()->handle()};

    VkCommandBuffer cmds[] = {top_cmd.handle()};

    std::vector<VkPipelineStageFlags> prepare_wait_stage_masks = std::vector<VkPipelineStageFlags>(VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, prepare_semaphores.size());

    VkSubmitInfo s_info{
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,

        .waitSemaphoreCount = static_cast<u32>(prepare_semaphores.size()),
        .pWaitSemaphores    = prepare_semaphores.data(),
        .pWaitDstStageMask  = prepare_wait_stage_masks.data(),

        .commandBufferCount = vke::array_len(cmds),
        .pCommandBuffers    = cmds,

        .signalSemaphoreCount = static_cast<u32>(present_wait_semaphores.size()),
        .pSignalSemaphores    = present_wait_semaphores.data(),

    };

    VK_CHECK(vkQueueSubmit(get_context()->get_graphics_queue(), 1, &s_info, fence));

    if (!m_window->surface()->present()) {
        VK_CHECK(vkDeviceWaitIdle(device()));
        m_window->surface()->recrate_swapchain();
    }

    m_frame_index = (m_frame_index + 1) % FRAME_OVERLAP;
}

RenderServer::~RenderServer() {
    assert(m_early_cleanup_called == true && "early_cleanup must be called!");
}

RenderServer::RenderServer() {}

void RenderServer::early_cleanup() {
    assert(m_early_cleanup_called == false && "early cleanup called twice!");
    VK_CHECK(vkDeviceWaitIdle(device()));
    // Wait for fences in order to ensure proper cleanup
    for (auto& framely : m_framely_data) {
        VkFence fence = framely.fence->handle();
        VK_CHECK(vkWaitForFences(device(), 1, &fence, true, 5E9));
    }

    m_early_cleanup_called = true;
}
} // namespace vke