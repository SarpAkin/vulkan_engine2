#pragma once

#include <vke/vke.hpp>

struct SDL_Window;

namespace vke {
class WindowSDL final : public Window {
public:
    WindowSDL(int width = 1280,int height = 720,const char* title = "untitled window");
    ~WindowSDL();

    void init_surface() override;
    void poll_events() override;

    void lock_mouse() override;
    void unlock_mouse() override;

private:
    SDL_Window* m_window          = nullptr;
};

} // namespace vke