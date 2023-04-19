#include "window.hpp"

#include "surface.hpp"

#include <SDL2/SDL.h>

namespace vke {
Window::~Window() {

    // m_surface = nullptr;
}

void Window::lock_mouse() {
    SDL_SetRelativeMouseMode(SDL_TRUE);
    SDL_ShowCursor(SDL_DISABLE);
    m_mouse_locked = true;
}

void Window::unlock_mouse() {
    SDL_SetRelativeMouseMode(SDL_FALSE);
    SDL_ShowCursor(SDL_ENABLE);
    m_mouse_locked = false;
}

} // namespace vke