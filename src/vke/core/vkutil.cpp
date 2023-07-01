#include "vkutil.hpp"

#include "../util.hpp"

#include "../util/arena_alloc.hpp"

#include <vulkan/vk_enum_string_helper.h>

#include <filesystem>
#include <iostream>
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

std::span<u32> compile_glsl_file(ArenaAllocator* alloc, const char* path) {
    auto glsl = read_file(path);

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
