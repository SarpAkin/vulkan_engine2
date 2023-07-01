#include "engine.hpp"

#include <bits/chrono.h>
#include <chrono>
#include <cstdint>
#include <fmt/core.h>
#include <memory>
#include <vector>
#include <vulkan/vulkan_core.h>

#include "../core/commandbuffer.hpp"
#include "../core/core.hpp"
#include "../core/descriptor_pool.hpp"
#include "../core/fence.hpp"
#include "../core/renderpass.hpp"
#include "../core/semaphore.hpp"
#include "../core/vkutil.hpp"
#include "../core/window_sdl.hpp"
#include "../fwd.hpp"
#include "material_manager.hpp"

namespace vke {

RenderEngine::RenderEngine(CoreConfig* base_config, bool headless) {
    CoreConfig config;
    if (base_config) {
        config = *base_config;
    }

    if (!headless) {
        m_primary_window = Window_SDL::create_window(1920, 1080, "engine");
        config.window    = m_primary_window.get();

        m_windows.push_back(m_primary_window.get());
    }

    m_core = std::make_unique<Core>(&config);

    m_material_manager = std::make_unique<MaterialManager>(this);

    for (int i = 0; i < FRAME_OVERLAP; ++i) {
        m_frame_data[i] = {
            .render_fence     = std::make_unique<Fence>(core(), true),
            .cmd              = std::make_unique<CommandBuffer>(core(), true),
            .render_semaphore = std::make_unique<Semaphore>(core()),
            .framely_pool     = std::make_unique<DescriptorPool>(core(), 100),
        };
    }
}

RenderEngine::~RenderEngine() {}

void RenderEngine::run() {

    auto start_time = std::chrono::high_resolution_clock::now();

    f64 time_counter  = 0;
    u32 frame_counter = 0;

    while (m_primary_window->is_open()) {
        frame();

        auto end_time = std::chrono::high_resolution_clock::now();

        m_delta_time = static_cast<f64>((end_time - start_time).count()) / 1E9; // divide by a billion
        start_time   = end_time;

        frame_counter++;
        m_total_frame_counter++;
        time_counter += m_delta_time;

        if (time_counter > 1.0) {
            time_counter -= 1.0;
            m_fps         = frame_counter;
            frame_counter = 0;
            fmt::print("fps {},delta time {}\n", m_fps, m_delta_time);
        }
    }

    for (int i = 0; i < FRAME_OVERLAP; i++) {
        m_frame_data[i].render_fence->wait();
    }
}

void RenderEngine::frame() {
#ifndef NDEBUG
    constexpr u64 MAX_FRAME_TIME = 10E9;
#else
    constexpr u64 MAX_FRAME_TIME = UINT64_MAX;
#endif

    m_primary_window->poll_events();

    FrameData& current_frame = get_current_frame_data();

    current_frame.render_fence->wait(MAX_FRAME_TIME);
    current_frame.render_fence->reset();

    CommandBuffer* cmd = current_frame.cmd.get();
    cmd->reset();
    cmd->begin();

    on_frame(*cmd);

    cmd->end();

    std::vector<VkSemaphore> prepare_semaphores;
    std::vector<VkSemaphore> present_wait_semaphores;

    std::vector<Window*> windows_to_present;
    for (auto& window : m_windows) {
        auto semaphore = window->surface()->get_prepare_semaphore();
        if (semaphore == nullptr) continue;

        prepare_semaphores.push_back(semaphore->handle());
        present_wait_semaphores.push_back(window->surface()->get_wait_semaphore()->handle());
        windows_to_present.push_back(window);
    }

    VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

    current_frame.render_semaphore->reset();

    VkSubmitInfo submit = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,

        .waitSemaphoreCount = static_cast<u32>(prepare_semaphores.size()),
        .pWaitSemaphores    = prepare_semaphores.data(),
        .pWaitDstStageMask  = &wait_stage,

        .commandBufferCount = 1,
        .pCommandBuffers    = &cmd->handle(),

        .signalSemaphoreCount = static_cast<u32>(present_wait_semaphores.size()),
        .pSignalSemaphores    = present_wait_semaphores.data(),
    };

    CommandBuffer* cmds[] = {cmd};
    current_frame.render_fence->submit(&submit, cmds);

    for (auto& window : windows_to_present) {
        window->surface()->present();
    }

    // m_primary_window->surface()->present(current_frame.render_semaphore.get());

    m_frame_index = (m_frame_index + 1) % FRAME_OVERLAP;
}

} // namespace vke