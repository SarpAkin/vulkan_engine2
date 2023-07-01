#pragma once

#include "../common.hpp"
#include "../core/core.hpp"
#include "../core/window_sdl.hpp"
#include "../fwd.hpp"

#include <memory>
#include <vector>

namespace vke {

class RenderEngine {
public:
    RenderEngine(CoreConfig* config = nullptr,bool headless = false);
    ~RenderEngine();

    Core* core() const { return m_core.get(); }
    VkDevice device() const { return core()->device(); }
    Window* window() const { return m_primary_window.get(); }

    void run();

    inline u32 get_frame_index() { return m_frame_index; }
    inline u32 get_frame_overlap() { return FRAME_OVERLAP; }

    inline f64 get_delta_time() { return m_delta_time; }
    DescriptorPool* get_framely_pool() { return get_current_frame_data().framely_pool.get(); }
    MaterialManager* get_material_manager() { return m_material_manager.get(); }

    u64 get_frames_since_start() { return m_total_frame_counter; };

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

    u64 m_total_frame_counter;

    f64 m_delta_time = 0.0;
    u32 m_fps        = 0;

    u32 m_frame_index = 0;

    struct FrameData {
        std::unique_ptr<Fence> render_fence;
        std::unique_ptr<CommandBuffer> cmd;
        std::unique_ptr<Semaphore> render_semaphore; // signaled after framely submitted cmd finishes.
        std::unique_ptr<DescriptorPool> framely_pool;
    } m_frame_data[FRAME_OVERLAP];

    std::unique_ptr<MaterialManager> m_material_manager;
};

} // namespace vke