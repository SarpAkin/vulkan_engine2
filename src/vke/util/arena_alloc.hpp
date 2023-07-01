#pragma once

#include "../common.hpp"

#include <span>
#include <cstring>


namespace vke {
class ArenaAllocator {
public:
    ArenaAllocator();
    ~ArenaAllocator();

    void* alloc(usize size);

    template <typename T>
    T* alloc(usize count) { return reinterpret_cast<T*>(alloc(count * sizeof(T))); }

    template <typename T>
    std::span<T> create_copy(std::span<const T> src) {
        T* dst = alloc<T>(src.size());
        memcpy(dst, src.data(), src.size_bytes());
        return std::span<T>(dst,src.size());
    }

    ArenaAllocator(const ArenaAllocator&)            = delete;
    ArenaAllocator& operator=(const ArenaAllocator&) = delete;

private:
    u8* m_base;
    u8* m_top;
    u8* m_cap;
};
} // namespace vke