#include "util.hpp"

#include <fstream>
#include <ios>
#include <iterator>
#include <vector>

std::vector<u8> read_file_binary(const char* name) {
    std::ifstream file(name, std::ios::ate | std::ios::binary);
    if (!file.is_open()) {
        throw std::runtime_error("failed to open file");
    }
    size_t fileSize = static_cast<size_t>(file.tellg());
    std::vector<u8> buffer(fileSize);
    file.seekg(0);
    file.read(reinterpret_cast<char*>(buffer.data()), fileSize);
    return buffer;
}

std::span<u32> cast_u8_to_span_u32(std::span<u8> span) {
    if (span.size() % sizeof(u32) != 0) {
        throw std::runtime_error("cast_u8_to_span_u32: span size is not multiple of sizeof(u32)");
    }

    return std::span<u32>(reinterpret_cast<u32*>(span.data()), span.size() / sizeof(u32));
}

std::string read_file(const char* name) {
    std::ifstream file(name);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open file: " + std::string(name));
    }

    return std::string(std::istreambuf_iterator<char>(file), {});
}
