#include "vkutil.hpp"

#include "../util.hpp"

#include "../util/arena_alloc.hpp"

#include <vulkan/vk_enum_string_helper.h>

#include <filesystem>
#include <iostream>
#include <regex>
#include <vector>

#include <shaderc/shaderc.hpp>

namespace vke {
namespace fs = std::filesystem;

const char* vk_result_string(VkResult res) {
    return string_VkResult(res);
}

bool is_depth_format(VkFormat format) {
    const static VkFormat depth_formats[] = {
        VK_FORMAT_D16_UNORM,
        VK_FORMAT_D32_SFLOAT,
        VK_FORMAT_D16_UNORM_S8_UINT,
        VK_FORMAT_D24_UNORM_S8_UINT,
        VK_FORMAT_D32_SFLOAT_S8_UINT,
    };

    for (auto df : depth_formats)
        if (df == format) return true;

    return false;
}

std::string resolve_includes(std::string& glsl, fs::path path) {
    path = path.parent_path();

    std::string final;

    std::regex pattern("(#include\\s+\"([^\"]+)\")");
    std::smatch match;

    auto it  = glsl.cbegin();
    auto end = glsl.cend();
    while (std::regex_search(it, end, match, pattern)) {
        final += std::string_view(it, match[0].first);
        auto header_path = path / match[2].str();
    
        auto header      = read_file(header_path.c_str());
        final += resolve_includes(header, header_path);
        it = match[0].second;
    }

    final += std::string_view(it, end);

    return final;
}

std::span<u32> compile_glsl_file(ArenaAllocator* alloc, const char* path) {
    auto glsl = read_file(path);
    glsl      = resolve_includes(glsl, path);

    shaderc_shader_kind kind = shaderc_glsl_infer_from_source;
    auto ext                 = fs::path(path).extension().string();

    if (ext == ".comp") kind = shaderc_glsl_compute_shader;
    if (ext == ".vert") kind = shaderc_glsl_vertex_shader;
    if (ext == ".frag") kind = shaderc_glsl_fragment_shader;

    shaderc::CompileOptions options;

    shaderc::Compiler compiler;
    auto result = compiler.CompileGlslToSpv(glsl, kind, path);

    auto status = result.GetCompilationStatus();
    if (status != shaderc_compilation_status_success) {
        throw std::runtime_error(result.GetErrorMessage());
    }

    return alloc->create_copy(std::span(result.begin(), result.end()));
}

} // namespace vke
