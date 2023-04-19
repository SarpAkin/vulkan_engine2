#pragma once

#include "common.hpp"
#include "core.hpp"
#include "fwd.hpp"
#include "window_sdl.hpp"

#include <memory>
#include <vector>

namespace vke {

class RenderEngine {
public:
    RenderEngine();
    ~RenderEngine();

    Core* core() const { return m_core.get(); }
    VkDevice device() const { return core()->device(); }
    Window* window() const { return m_primary_window.get(); }

    void run();

    inline u32 get_frame_index() { return m_frame_index; }
    inline u32 get_frame_overlap() { return FRAME_OVERLAP; }

    inline f64 get_delta_time() { return m_delta_time; }

    DescriptorPool* get_framely_pool() { return get_current_frame_data().framely_pool.get(); }

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
        std::unique_ptr<Semaphore> render_semaphore; // signaled after framely submitted cmd finishes.
        std::unique_ptr<DescriptorPool> framely_pool;
    } m_frame_data[FRAME_OVERLAP];
};

} // namespace vke