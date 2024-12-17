#include "window_sdl.hpp"


#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>

#include <SDL2/SDL_keycode.h>

#include <memory>
#include <vulkan/vulkan_core.h>

namespace vke {

static bool sld_initialized = false;
static void init_sdl() {
    if (sld_initialized) return;

    SDL_Init(SDL_INIT_VIDEO);
    atexit(+[] { SDL_Quit(); });
}

WindowSDL::WindowSDL(int width,int height,const char* title) {
    m_width = width;
    m_height = height;

    init_sdl();

    m_window = SDL_CreateWindow(title, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, m_width, m_height, (SDL_WindowFlags)(SDL_WINDOW_VULKAN));
}

void WindowSDL::init_surface() {
    VkSurfaceKHR surface;
    SDL_Vulkan_CreateSurface(m_window, VulkanContext::get_context()->get_instance(), &surface);

    m_surface = std::make_unique<Surface>(surface, this);
}

void WindowSDL::poll_events() {
    SDL_Event e;

    m_mouse_input.delta_x = 0;
    m_mouse_input.delta_y = 0;

    while (SDL_PollEvent(&e)) {
        // if (m_imgui_manager) {
        //     m_imgui_manager->process_sdl_event(e);
        // }

        switch (e.type) {
            // case SDL_WINDOWEVENT_CLOSE:

        case SDL_QUIT:
            m_is_open = false;
            break;
        case SDL_KEYDOWN:
            m_key_states[e.key.keysym.sym] = true;
            if (auto it = m_key_callbacks.find(e.key.keysym.sym); it != m_key_callbacks.end())
                it->second();

            break;
        case SDL_KEYUP:
            m_key_states[e.key.keysym.sym] = false;
            break;

        case SDL_MOUSEBUTTONDOWN:
            // m_mouse_button_states[e.button.button] = true;
            if (auto it = m_mouse_callbacks.find(e.button.button); it != m_mouse_callbacks.end())
                it->second();

            break;

        // case SDL_MOUSEBUTTONUP:
        //     m_mouse_button_states[e.button.button] = false;
        //     break;
        case SDL_MOUSEMOTION:
            m_mouse_input.delta_x = m_mouse_input.xpos - e.motion.x;
            m_mouse_input.delta_y = m_mouse_input.ypos - e.motion.y;

            m_mouse_input.xpos = e.motion.x;
            m_mouse_input.ypos = e.motion.y;
            break;
        }
    }

    // if (m_imgui_manager) {
    //     m_imgui_manager->new_frame();
    // }

    if (m_mouse_locked) {

        SDL_WarpMouseInWindow(m_window, width() / 2, height() / 2);
        m_mouse_input.xpos = (float)width() / 2;
        m_mouse_input.ypos = (float)height() / 2;
    }
}

WindowSDL::~WindowSDL() {
    SDL_DestroyWindow(m_window);
};

void WindowSDL::lock_mouse() {
    SDL_SetRelativeMouseMode(SDL_TRUE);
    m_mouse_locked = true;
}

void WindowSDL::unlock_mouse() {
    SDL_SetRelativeMouseMode(SDL_FALSE);
    m_mouse_locked = false;
}

} // namespace vke