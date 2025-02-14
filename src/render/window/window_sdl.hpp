#pragma once

#include <vke/vke.hpp>

struct SDL_Window;

namespace vke {
class ImguiManager;

class WindowSDL final : public Window {
public:
    WindowSDL(int width = 1280, int height = 720, const char* title = "untitled window");
    ~WindowSDL();

    void init_surface() override;
    void poll_events() override;

    void lock_mouse() override;
    void unlock_mouse() override;

    SDL_Window* handle() { return m_window; }
    
    void set_imgui_manager(ImguiManager* imgui_manager) { m_imgui_manager = imgui_manager; }
private:
    SDL_Window* m_window = nullptr;
    ImguiManager* m_imgui_manager;
};

} // namespace vke