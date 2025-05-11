#pragma once

#include "common.hpp"
#include "fwd.hpp"

#include <vke/fwd.hpp>
#include <memory>
#include <vulkan/vulkan.h>

namespace vke{


class GPUTimingSystem{
public:
    GPUTimingSystem(vke::RenderServer* rs);
    ~GPUTimingSystem();

    void begin_frame(vke::CommandBuffer& cmd);
    void end_frame(vke::CommandBuffer& cmd);

    void timestamp(vke::CommandBuffer& cmd, std::string_view label, VkPipelineStageFlagBits stage);
private:
    GPUTimer* get_current_timer();
    void debug_menu();

private:
    std::unique_ptr<vke::GPUTimer> m_timers[FRAME_OVERLAP];
    vke::RenderServer* m_render_server = nullptr;
    bool m_enabled = true;
};


}