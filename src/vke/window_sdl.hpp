#pragma once

#include <memory>
#include <string>

#include "common.hpp"
#include "fwd.hpp"
#include "window.hpp"

struct SDL_Window;

namespace vke {

class ImguiManager;

class Window_SDL : public Window {
public:
    void init_surface(Core* core) override;

    static std::unique_ptr<Window_SDL> create_window(u32 width, u32 height, const char* title) {
        return std::unique_ptr<Window_SDL>(new Window_SDL(width, height, title));
    }

    void poll_events() override;

    SDL_Window* handle() { return m_window; }

    void register_imgui_manager(ImguiManager* manager) { m_imgui_manager = manager; };

private:
    Window_SDL(u32 width, u32 height, const char* title) {
        m_width  = width;
        m_height = height;
        m_title  = title;

        init();
    }

    void init();

private:
    SDL_Window* m_window          = nullptr;
    ImguiManager* m_imgui_manager = nullptr;
};

} // namespace vke
