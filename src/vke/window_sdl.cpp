#include "window_sdl.hpp"

#include "core.hpp"
#include "fwd.hpp"
#include "surface.hpp"

#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>
#include <SDL_video.h>
#include <memory>
#include <vulkan/vulkan_core.h>

#include "imgui/imgui_manager.hpp"

namespace vke {

static bool sld_initialized = false;
static void init_sdl() {
    if (sld_initialized) return;

    SDL_Init(SDL_INIT_VIDEO);
    atexit(+[] { SDL_Quit(); });
}

void Window_SDL::init() {
    init_sdl();

    m_window = SDL_CreateWindow("", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, m_width, m_height, (SDL_WindowFlags)(SDL_WINDOW_VULKAN));
}

void Window_SDL::init_surface(Core* core) {
    VkSurfaceKHR surface;
    SDL_Vulkan_CreateSurface(m_window, core->instance(), &surface);

    m_surface = std::make_unique<Surface>(core, surface);
}

void Window_SDL::poll_events() {
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        if (m_imgui_manager) {
            m_imgui_manager->process_sdl_event(e);
        }

        switch (e.type) {
            // case SDL_WINDOWEVENT_CLOSE:

        case SDL_QUIT:
            m_is_open = false;
            break;
            // case SDL_KEYDOWN:
            //     m_keystates[e.key.keysym.sym] = true;
            //     if (auto it = m_on_key_down.find(e.key.keysym.sym); it != m_on_key_down.end())
            //         for (auto& [_, f] : it->second)
            //             f();
            //     break;
            // case SDL_KEYUP:
            //     m_keystates[e.key.keysym.sym] = false;
            //     break;

            // case SDL_MOUSEBUTTONDOWN:
            //     m_mouse_button_states[e.button.button] = true;
            //     if (auto it = m_on_mouse_click.find(e.button.button); it != m_on_mouse_click.end())
            //         for (auto& [_, f] : it->second)
            //             f();

            //     break;

            // case SDL_MOUSEBUTTONUP:
            //     m_mouse_button_states[e.button.button] = false;
            //     break;
            // case SDL_MOUSEMOTION:

            //     m_mouse_delta_x = m_mouse_x - e.motion.x;
            //     m_mouse_delta_y = m_mouse_y - e.motion.y;

            //     m_mouse_x = e.motion.x;
            //     m_mouse_y = e.motion.y;
            //     break;
        }
    }

    if (m_imgui_manager) {
        m_imgui_manager->new_frame();
    }
}

} // namespace vke