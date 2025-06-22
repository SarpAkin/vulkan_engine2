#pragma once

#include <memory>

#include <any>
#include <vke/fwd.hpp>
#include <vke/vke.hpp>

#include "fwd.hpp"

#include "common.hpp" // IWYU pragma: export
#include "render/iobject_renderer.hpp"

#include <entt/entt.hpp>

namespace vke {
class ObjectRenderer;
class ImguiManager;

class RenderServer : protected vke::DeviceGetter {
public:
    struct FrameArgs {
        vke::CommandBuffer* main_pass_cmd;
        // vke::CommandBuffer* pre_pass_compute_cmd;
        vke::CommandBuffer* primary_cmd; // main_pass_cmd is called after
    };

    struct CommandSubmitInfo {
        vke::SlimVec<vke::CommandBuffer*> cmd;
        vke::SlimVec<VkSemaphore> signal_semaphore;
        vke::SlimVec<VkSemaphore> wait_semaphores;
    };

    RenderServer();
    ~RenderServer();

    void init();
    void run();
    void early_cleanup();

    IPipelineLoader* get_pipeline_loader() { return m_pipeline_loader.get(); }
    Window* get_window() { return m_window.get(); }
    ObjectRenderer* get_object_renderer() { return m_object_renderer.get(); }
    LineDrawer* get_line_drawer() { return m_line_drawer.get(); }
    GPUTimingSystem* get_gpu_timing_system() { return m_timing_system.get(); }

    void frame(std::function<void(FrameArgs& args)> render_function);
    bool is_running() { return m_running && m_window->is_open(); }

    int get_frame_index() const { return m_frame_index; }

    DescriptorPool* get_descriptor_pool() { return m_descriptor_pool.get(); }
    CommandPool* get_framely_command_pool() { return m_framely_data[m_frame_index].cmd_pool.get(); }

    void submit_cmd(CommandSubmitInfo&& info) {
        m_main_queue_submit_infos.push_back(std::move(info));
    }

    auto& get_any_storage() { return m_custom_any_storage; }
    const auto& get_any_storage() const { return m_custom_any_storage; }

    VkDevice get_device() const { return device(); }

private:
private:
    std::unique_ptr<vke::Window> m_window;
    std::unique_ptr<vke::Renderpass> m_window_renderpass;
    std::unique_ptr<vke::IPipelineLoader> m_pipeline_loader;
    std::unique_ptr<vke::ObjectRenderer> m_object_renderer;
    std::unique_ptr<vke::DescriptorPool> m_descriptor_pool;
    std::unique_ptr<vke::ImguiManager> m_imgui_manager;
    std::unique_ptr<vke::LineDrawer> m_line_drawer;
    std::unique_ptr<vke::GPUTimingSystem> m_timing_system;

    std::unordered_map<std::string, std::any> m_custom_any_storage;

    bool m_running              = true;
    bool m_early_cleanup_called = false;
    int m_frame_index           = 0;

    struct FramelyData {
        std::unique_ptr<vke::CommandPool> cmd_pool;
        std::unique_ptr<vke::CommandBuffer> cmd, main_pass_cmd;
        std::unique_ptr<vke::Fence> fence;
    };

    std::vector<FramelyData> m_framely_data;
    std::vector<CommandSubmitInfo> m_main_queue_submit_infos;
};

} // namespace vke