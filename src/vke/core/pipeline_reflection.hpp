#pragma once

#include <optional>
#include <vector>
#include <vulkan/vulkan_core.h>

#include "../fwd.hpp"
#include "../util/arena_alloc.hpp"

struct SpvReflectShaderModule;

namespace vke {

class BufferRefletion;

class PipelineReflection {
public:
    struct LayoutBuild {
        VkPipelineLayout layout;
        std::vector<VkDescriptorSetLayout> dset_layouts;
        VkShaderStageFlagBits push_stages;
    };

    PipelineReflection() {}

    VkShaderStageFlagBits add_shader_stage(std::span<const u32> spirv);

    LayoutBuild build_pipeline_layout(Core* core) const;
    std::optional<BufferRefletion> reflect_buffer(u32 set, u32 binding) const;

private:
    ArenaAllocator m_alloc;

    struct ShaderStage {
        std::span<u32> spirv;
        VkShaderStageFlagBits stage;
        SpvReflectShaderModule* module;
    };
    std::vector<ShaderStage> m_shaders;
};
} // namespace vke