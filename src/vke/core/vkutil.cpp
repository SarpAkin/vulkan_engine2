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

class ShadercIncluder : public shaderc::CompileOptions::IncluderInterface {
public:
    ShadercIncluder(ArenaAllocator* _arena){
        arena = _arena;
    }

    ArenaAllocator* arena;

    shaderc_include_result* GetInclude(const char* requested_source, shaderc_include_type type, const char* requesting_source, size_t include_depth) override {
        if (type == shaderc_include_type_standard) return nullptr;

        fs::path path = fs::path(requesting_source).parent_path() / requested_source;

        auto content = read_file(arena, path.c_str());

        usize pathc_len;
        const char* pathc = arena->create_str_copy(path.c_str(), &pathc_len);

        return arena->create_copy(shaderc_include_result{
            .source_name        = pathc,
            .source_name_length = pathc_len,
            .content            = content.begin(),
            .content_length     = content.size(),
        });
    }

    void ReleaseInclude(shaderc_include_result* data) override {
    }
};

std::span<u32> compile_glsl_file(ArenaAllocator* alloc, const char* path, ShaderCompileOptions* options) {
    ArenaAllocator scratch;

    auto glsl = read_file(&scratch, path);
    // glsl      = resolve_includes(glsl, path);

    shaderc_shader_kind kind = shaderc_glsl_infer_from_source;
    auto ext                 = fs::path(path).extension().string();

    if (ext == ".comp") kind = shaderc_glsl_compute_shader;
    if (ext == ".vert") kind = shaderc_glsl_vertex_shader;
    if (ext == ".frag") kind = shaderc_glsl_fragment_shader;

    shaderc::CompileOptions shaderc_options;
    // shaderc_options.SetOptimizationLevel(shaderc_optimization_level_performance);
    shaderc_options.SetIncluder(std::make_unique<ShadercIncluder>(&scratch));

    if (options) {
        for (const auto& [d, v] : options->defines) {
            shaderc_options.AddMacroDefinition(d, v);
        }
    }

    shaderc::Compiler compiler;
    auto result = compiler.CompileGlslToSpv(glsl.begin(),glsl.size(), kind, path, shaderc_options);

    auto status = result.GetCompilationStatus();
    if (status != shaderc_compilation_status_success) {
        throw std::runtime_error(result.GetErrorMessage());
    }

    return alloc->create_copy(std::span(result.begin(), result.end()));
}

} // namespace vke
