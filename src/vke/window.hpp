#pragma once

#include "surface.hpp"


#include "common.hpp"
#include "fwd.hpp"

#include <memory>
#include <string>

namespace vke {
class Window {
public:
    inline u32 width() const { return m_width; }
    inline u32 height() const { return m_height; }
    inline bool is_open() const { return m_is_open; }

    inline Surface* surface() { return m_surface.get(); }
    virtual void init_surface(Core* core) = 0;

    virtual void poll_events() = 0;

    virtual ~Window();
protected:
    u32 m_width = 0, m_height = 0;
    std::string m_title;
    bool m_is_open = true;

    std::unique_ptr<Surface> m_surface;
};
} // namespace vke