#pragma once

#include "../common.hpp"
#include "../fwd.hpp"
#include "../vk_resource.hpp"
#include "../window.hpp"

union SDL_Event;

namespace vke {
class ImguiManager : private Resource {
public:
    ImguiManager(Core*, Window*,Renderpass* renderpass,u32 subpass_index);
    ~ImguiManager();

    void new_frame();
    void process_sdl_event(SDL_Event& event);
    void flush_frame(CommandBuffer& cmd);

private:
    VkDescriptorPool m_imgui_pool;

    Window_SDL* m_window;
};
} // namespace vke