#pragma once

#include <memory>

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
    RenderServer();
    ~RenderServer();

    void init();
    void run();

    IPipelineLoader* get_pipeline_loader() { return m_pipeline_loader.get(); }
    Window* get_window() { return m_window.get(); }
    ObjectRenderer* get_object_renderer() { return m_object_renderer.get(); }

    void frame(std::function<void(vke::CommandBuffer& cmd)> render_function);
    bool is_running() { return m_running && m_window->is_open(); }

    int get_frame_index() const { return m_frame_index; }

    DescriptorPool* get_descriptor_pool(){return m_descriptor_pool.get();}

private:
private:
    std::unique_ptr<vke::Window> m_window;
    std::unique_ptr<vke::Renderpass> m_window_renderpass;
    std::unique_ptr<vke::IPipelineLoader> m_pipeline_loader;
    std::unique_ptr<vke::ObjectRenderer> m_object_renderer;
    std::unique_ptr<vke::DescriptorPool> m_descriptor_pool;
    std::unique_ptr<vke::ImguiManager> m_imgui_manager;

    bool m_running    = true;
    int m_frame_index = 0;

    struct FramelyData {
        std::unique_ptr<vke::CommandBuffer> cmd;
        std::unique_ptr<vke::Fence> fence;
    };

    std::vector<FramelyData> m_framely_data;
};

} // namespace vke