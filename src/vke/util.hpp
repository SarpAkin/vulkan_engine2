#pragma once

#include <alloca.h>
#include <cassert>
#include <optional>
#include <span>
#include <vector>

#include "common.hpp"

auto map_vec(auto&& vector, auto&& f) -> std::vector<decltype(f(*vector.begin()))> {
    std::vector<decltype(f(*vector.begin()))> results;
    results.reserve(vector.size());
    for (const auto& element : vector) {
        results.push_back(f(element));
    }

    return results;
}

template <typename T>
auto map_optional(const std::optional<T>& opt, auto&& func) -> std::optional<decltype(func(*opt))> {
    if (opt) {
        std::make_optional(func(*opt));
    }
    return std::nullopt;
}

#define ARRAY_LEN(arr) (sizeof(arr) / sizeof(arr[0]))

#define TODO() assert(!"TODO!")

// auto map_vec_to_ptr_buffer(void* ptr,auto&& vector, auto&& f) -> std::vector<decltype(f(vector[0]))> {
//     using T    = decltype(f(*vector.begin()));

//     for (const auto& element : vector) {
//         results.push_back(f(element));
//     }

//     return results;
// }

// returns an std::span allocated from alloca
#define ALLOCA_ARR(T, N) std::span(reinterpret_cast<T*>(alloca(sizeof(T) * N)), static_cast<usize>(N))

#define ALLOCA_ARR_WITH_VALUE(T, N, value) ({                                                  \
    auto span = std::span(reinterpret_cast<T*>(alloca(sizeof(T) * N)), static_cast<usize>(N)); \
    for (auto& item : span) {                                                                  \
        item = value;                                                                          \
    }                                                                                          \
    span;                                                                                      \
})

// returns an std::span allocated from alloca
#define MAP_VEC_ALLOCA(vector, f...) ({                                   \
    using T    = decltype(f(*vector.begin()));                            \
    T* results = reinterpret_cast<T*>(alloca(sizeof(T) * vector.size())); \
    T* it      = results;                                                 \
    for (auto& element : vector) {                                        \
        *(it++) = f(element);                                             \
    }                                                                     \
    std::span(results, it);                                               \
})

std::vector<u8> read_file_binary(const char* name);

std::span<u32> cast_u8_to_span_u32(std::span<u8> span);
