#pragma once

#include "../common.hpp"
#include "../fwd.hpp"
#include "../core/vk_resource.hpp"
#include "../core/window.hpp"

#include "../engine/irender_system.hpp"

union SDL_Event;

namespace vke {
class ImguiManager : private Resource,public IRenderSystem {
public:
    ImguiManager(Core*, Window*,Renderpass* renderpass,u32 subpass_index);
    ~ImguiManager();

    void new_frame();
    void process_sdl_event(SDL_Event& event);
    void flush_frame(CommandBuffer& cmd);

    void render(vke::IRenderTarget* render_target) override;
    void on_subscribe(vke::IRenderTarget*) override;

private:
    VkDescriptorPool m_imgui_pool;

    Window_SDL* m_window;
};
} // namespace vke