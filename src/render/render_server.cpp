#include "render_server.hpp"

#include "mesh/mesh_util.hpp"
#include "mesh/shapes.hpp"
#include "render/debug/line_drawer.hpp"
#include "render/imgui/imgui_manager.hpp"
#include "render/object_renderer/object_renderer.hpp"
#include "render/object_renderer/resource_manager.hpp"
#include "scene/camera.hpp"
#include "scene/components/transform.hpp"
#include "window/window_sdl.hpp"

#include "render/debug/gpu_timing_system.hpp"

#include <filesystem>
#include <vke/pipeline_loader.hpp>
#include <vke/util.hpp>
#include <vke/vke.hpp>

#include <chrono>

namespace vke {

void RenderServer::init() {
    vke::ContextConfig config{
        .app_name    = "app0",
        .features1_0 = {
            .shaderFloat64         = true,
            .shaderInt64           = true,
            .shaderInt16           = true,
            .sparseBinding         = true,
            .sparseResidencyBuffer = true,
        },
        .features1_2 = {
            .shaderInt8          = true,
            .samplerFilterMinmax = true,
        },
    };

    vke::VulkanContext::init(config);

    m_descriptor_pool = std::make_unique<DescriptorPool>();

    m_window = std::make_unique<WindowSDL>();
    m_window->init_surface();

    m_window_renderpass = vke::make_simple_windowed_renderpass(m_window.get(), true);

    m_imgui_manager = std::make_unique<ImguiManager>(m_window.get(), m_window_renderpass.get(), 0);
    dynamic_cast<WindowSDL*>(m_window.get())->set_imgui_manager(m_imgui_manager.get());

    fs::path vke_engine_path = "submodules/vke_engine/"; 

    m_pipeline_loader = vke::IPipelineLoader::make_debug_loader({"./src/",vke_engine_path / "src/render/shader"});

    auto pg_provider = std::make_unique<vke::PipelineGlobalsProvider>();
    pg_provider->subpasses.emplace("vke::default_forward", std::make_unique<SubpassDetails>(*m_window_renderpass->get_subpass(0)));
    pg_provider->vertex_input_descriptions.emplace("vke::default_mesh", vke::make_default_vertex_layout());
    pg_provider->shader_compiler->add_system_include_dir("submodules/vke_engine/src/render/shader/include/");

    m_pipeline_loader->set_pipeline_globals_provider(std::move(pg_provider));

    for (int i = 0; i < FRAME_OVERLAP; i++) {
        auto _pool = std::make_unique<vke::CommandPool>();
        auto* pool = _pool.get();
        m_framely_data.push_back({
            .cmd_pool      = std::move(_pool),
            .cmd           = pool->allocate(),
            .main_pass_cmd = pool->allocate(false),
            .fence         = std::make_unique<vke::Fence>(true),
        });
    }

    m_object_renderer = std::make_unique<ObjectRenderer>(this);

    m_object_renderer->get_resource_manager()->set_subpass_type("vke::default_forward", MaterialSubpassType::FORWARD);
    // Creating a cube model
    //  auto materialID = m_object_renderer->create_material("vke::default", {}, "vke::default_material");
    //  auto meshID     = m_object_renderer->create_mesh(std::move(*vke::make_cube()));
    //  m_object_renderer->create_model(meshID, materialID,"cube");

    m_line_drawer = std::make_unique<vke::LineDrawer>(this);

    m_timing_system = std::make_unique<vke::GPUTimingSystem>(this);
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

    m_timing_system->begin_frame(top_cmd);

    auto args = FrameArgs{
        .main_pass_cmd = &main_renderpass_pass_cmd,
        .primary_cmd   = &top_cmd,
        // .pre_pass_compute_cmd = &pre_pass_comput_cmd,
    };

    render_function(args);

    m_timing_system->timestamp(top_cmd, "Imgui flush", VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);

    m_imgui_manager->flush_frame(main_renderpass_pass_cmd);

    main_renderpass_pass_cmd.end();

    m_window_renderpass->set_external(true);
    m_window_renderpass->begin(top_cmd);

    top_cmd.execute_secondaries(&main_renderpass_pass_cmd);

    m_window_renderpass->end(top_cmd);

    m_timing_system->end_frame(top_cmd);

    top_cmd.end();

    std::vector<VkSemaphore> prepare_semaphores      = {m_window->surface()->get_prepare_semaphore()->handle()};
    std::vector<VkSemaphore> present_wait_semaphores = {m_window->surface()->get_wait_semaphore()->handle()};

    VkCommandBuffer cmds[] = {top_cmd.handle()};

    std::vector<VkPipelineStageFlags> prepare_wait_stage_masks = std::vector<VkPipelineStageFlags>(VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, prepare_semaphores.size());

    auto submit_info_datas = vke::map_vec2small_vec(m_main_queue_submit_infos, [&](const CommandSubmitInfo& info) {
        std::vector<VkSemaphore> wait_semaphores;
        wait_semaphores.insert(wait_semaphores.end(), info.wait_semaphores.begin(), info.wait_semaphores.end());

        auto cmds = vke::map_vec(info.cmd, [&](CommandBuffer* cmd) {
            wait_semaphores.insert(wait_semaphores.end(), cmd->get_wait_semaphores().begin(), cmd->get_wait_semaphores().end());
            return cmd->handle();
        });

        return std::make_tuple(std::move(cmds), wait_semaphores);
    });

    vke::SmallVec<VkSubmitInfo, 8UL> submit_infos;
    submit_infos.reserve(submit_info_datas.size() + 1);

    for (int i = 0; i < submit_info_datas.size(); i++) {
        const auto& [cmds, waits] = submit_info_datas[i];
        auto& signals             = m_main_queue_submit_infos[i].signal_semaphore;

        submit_infos.push_back(VkSubmitInfo{
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,

            .waitSemaphoreCount   = static_cast<u32>(waits.size()),
            .pWaitSemaphores      = waits.data(),
            .commandBufferCount   = static_cast<u32>(cmds.size()),
            .pCommandBuffers      = cmds.data(),
            .signalSemaphoreCount = static_cast<u32>(signals.size()),
            .pSignalSemaphores    = signals.data(),
        });
    }

    submit_infos.push_back(VkSubmitInfo{
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,

        .waitSemaphoreCount = static_cast<u32>(prepare_semaphores.size()),
        .pWaitSemaphores    = prepare_semaphores.data(),
        .pWaitDstStageMask  = prepare_wait_stage_masks.data(),

        .commandBufferCount = vke::array_len(cmds),
        .pCommandBuffers    = cmds,

        .signalSemaphoreCount = static_cast<u32>(present_wait_semaphores.size()),
        .pSignalSemaphores    = present_wait_semaphores.data(),
    });

    VK_CHECK(vkQueueSubmit(get_context()->get_graphics_queue(), submit_infos.size(), submit_infos.data(), fence));
    m_main_queue_submit_infos.clear();

    if (!m_window->surface()->present()) {
        VK_CHECK(vkDeviceWaitIdle(device()));
        m_window->surface()->recrate_swapchain();
    } else if (!is_equal(m_window->extend(), m_window->surface()->extend())) {
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