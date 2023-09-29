#include "function_timer.hpp"

#include <fmt/core.h>
#include <mutex>
#include <unordered_map>

class TimingManager {
public:
    void function_time(const char* function, double time) {
        std::lock_guard<std::mutex> guard(m_lock);

        m_stats[function].total_time += time;
        m_stats[function].invocation_count += 1;
    }

    ~TimingManager() {
        for (auto& [fname, stats] : m_stats) {
            printf("[Function Timer] function: %s took %.4f ms on avarage to run. invocation count = %ld \n", fname, stats.total_time / stats.invocation_count, stats.invocation_count);
        }
    }

    struct FunctionStats {
        double total_time         = 0.0;
        uint64_t invocation_count = 0;
    };

private:
    std::mutex m_lock;
    std::unordered_map<const char*, FunctionStats> m_stats;
} man;

namespace vke {
namespace impl {
FunctionTimer::FunctionTimer(const char* func_name) {
    m_start     = std::chrono::steady_clock::now();
    m_func_name = func_name;
}

FunctionTimer ::~FunctionTimer() {
    double elapsed_time = ((double)(std::chrono::steady_clock::now() - m_start).count()) / 1000000.0;
    man.function_time(m_func_name, elapsed_time);
    // fmt::print("[Function Timer] function: {} took {} ms to run\n", m_func_name, elapsed_time);
}
} // namespace impl

} // namespace vke
