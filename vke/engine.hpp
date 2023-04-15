#pragma once

#include "common.hpp"
#include "core.hpp"
#include "fwd.hpp"
#include "window_sdl.hpp"

#include <memory>
#include <vector>

namespace vke {

constexpr usize FRAME_OVERLAP = 2;

class RenderEngine {
public:
    RenderEngine();
    ~RenderEngine();

    Core* core() const { return m_core.get(); }
    VkDevice device() const { return core()->device(); }
    Window* window() const { return m_primary_window.get(); }

    void run();

protected:
    virtual void on_frame(CommandBuffer& cmd){};

private:
    struct FrameData;
    inline FrameData& get_current_frame_data() { return m_frame_data[m_frame_index]; }

    void frame();

private:
    std::unique_ptr<Core> m_core;
    std::unique_ptr<Window_SDL> m_primary_window;
    std::vector<Window*> m_windows;

    f64 m_delta_time = 0.0;
    u32 m_fps        = 0;

    u32 m_frame_index = 0;

    struct FrameData {
        std::unique_ptr<Fence> render_fence;
        std::unique_ptr<CommandBuffer> cmd;
        std::unique_ptr<Semaphore> render_semaphore; //signaled after framely submitted cmd finishes.  
    } m_frame_data[FRAME_OVERLAP];

};

} // namespace vke