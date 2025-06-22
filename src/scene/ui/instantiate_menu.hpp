#pragma once

#include "fwd.hpp"
#include "ui/imenu.hpp"
#include <string>

namespace vke {

class InstantiateMenu : public IMenu {
public:
    InstantiateMenu(GameEngine* game_engine) {
        m_game_engine = game_engine;
    }

    void draw_menu() override;

private:
    GameEngine* m_game_engine = nullptr;

    float m_light_range = 5.f, m_light_strength = 2.f;
    float m_picker_color[4] = {};
    std::string m_selected_prefab;
    char m_coord_input_buffer[128] = "r(0,0,0) (1,1,1) (0,0,0)";
};

} // namespace vke