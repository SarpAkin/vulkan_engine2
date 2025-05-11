#include "gpu_timing_system.hpp"

#include <imgui.h>
#include <vke/util.hpp>

#include "render/render_server.hpp"

namespace vke {

GPUTimingSystem::GPUTimingSystem(vke::RenderServer* rs) {
    m_render_server = rs;

    for (auto& t : m_timers) {
        t = std::make_unique<vke::GPUTimer>(true);
    }
}
GPUTimingSystem::~GPUTimingSystem() {}

void GPUTimingSystem::begin_frame(vke::CommandBuffer& cmd) {
    debug_menu();

    auto* timer = get_current_timer();
    timer->set_enabled(m_enabled);

    timer->reset(cmd);
    timer->begin_frame(cmd);
}

void GPUTimingSystem::end_frame(vke::CommandBuffer& cmd) {
    get_current_timer()->end_frame(cmd);
}

void GPUTimingSystem::debug_menu() {
    auto* timer = get_current_timer();
    timer->query_results();
    timer->sort_results();

    auto labels = timer->get_labels();

    if (ImGui::Begin("Gpu Timers", &m_enabled) && !labels.empty()) {
        ImGui::Text("total time: %.3lf", timer->get_delta_time_in_miliseconds(0, labels.size() - 1));
        ImGui::Separator();

        for (int i = 0; i < labels.size(); i++) {
            double time = i < labels.size() - 1 ? timer->get_delta_time_in_miliseconds(i, i + 1) : 0.0;

            ImGui::Text("%s: %.3lf ms", labels[i].label.c_str(), time);
        }
    }

    ImGui::End();
}

void GPUTimingSystem::timestamp(vke::CommandBuffer& cmd, std::string_view label, VkPipelineStageFlagBits stage) { get_current_timer()->timestamp(cmd, label, stage); }

GPUTimer* GPUTimingSystem::get_current_timer() { return m_timers[m_render_server->get_frame_index()].get(); }

} // namespace vke