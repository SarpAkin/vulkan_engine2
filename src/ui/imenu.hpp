#pragma once

namespace vke {

class IMenu {
public:
    ~IMenu() {}

public:
    virtual void draw_menu() = 0;
};

} // namespace vke
