#pragma once

#include <vke/vke.hpp>

union SDL_Event;

namespace vke {
class WindowSDL;

class ImguiManager : vke::DeviceGetter {
public:
    ImguiManager(Window* window, Renderpass* renderpass, u32 subpass_index);
    ~ImguiManager();

    void new_frame();
    void process_sdl_event(SDL_Event& event);
    void flush_frame(CommandBuffer& cmd);

private:
    VkDescriptorPool m_imgui_pool;

    WindowSDL* m_window;
};
} // namespace vke