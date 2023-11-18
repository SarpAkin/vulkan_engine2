#pragma once

#include <cassert>
#include <cstddef>
#include <memory>
#include <optional>
#include <span>
#include <unordered_map>
#include <vector>
#include <vulkan/vulkan_core.h>

#include "../util/arena_alloc.hpp"

#include "../common.hpp"
#include "../fwd.hpp"
#include "pipeline.hpp"
#include "vk_resource.hpp"

struct SpvReflectBlockVariable;

namespace vke {

class PipelineReflection;

class PipelineLayoutBuilder {
public:
    template <typename T>
    PipelineLayoutBuilder& add_push_constant(VkShaderStageFlags stage) {
        m_push_constants.push_back(VkPushConstantRange{
            .stageFlags = stage,
            .offset     = 0,
            .size       = sizeof(T),
        });

        return *this;
    }

    PipelineLayoutBuilder& add_push_constant(VkShaderStageFlags stage, u32 size) {
        m_push_constants.push_back(VkPushConstantRange{
            .stageFlags = stage,
            .offset     = 0,
            .size       = size,
        });

        return *this;
    }

    inline PipelineLayoutBuilder& add_set_layout(VkDescriptorSetLayout layout) {
        m_set_layouts.push_back(layout);

        return *this;
    }

    VkPipelineLayout build(VkDevice device);

private:
    std::vector<VkPushConstantRange> m_push_constants;
    std::vector<VkDescriptorSetLayout> m_set_layouts;
};

class PipelineBuilderBase : protected Resource {
public:
    PipelineBuilderBase(Core* core);
    ~PipelineBuilderBase();

    // 0 is for auto
    void add_shader_stage(u32* spirv_code, usize spirv_len, VkShaderStageFlagBits stage = (VkShaderStageFlagBits)0);
    void add_shader_stage(std::span<u8> span, VkShaderStageFlagBits stage = (VkShaderStageFlagBits)0) {
        assert(span.size() % 4 == 0);
        add_shader_stage(reinterpret_cast<u32*>(span.data()), span.size() / 4, stage);
    }
    void add_shader_stage(std::span<u32> span, VkShaderStageFlagBits stage = (VkShaderStageFlagBits)0) { add_shader_stage(span.data(), span.size(), stage); };
    void add_shader_stage(std::string_view spirv_path);
    void set_layout_builder(PipelineLayoutBuilder* builder) { m_layout_builder = builder; }
    void set_pipeline_cache(VkPipelineCache cache) { m_pipeline_cache = cache; }

    const PipelineReflection* get_reflection() const { return m_reflection.get(); };

protected:
    std::unique_ptr<PipelineReflection> m_reflection;

    struct ShaderDetails {
        u32* spirv_code;
        u32 spirv_len;
        VkShaderStageFlagBits stage;
        VkShaderModule module;
    };

    std::vector<ShaderDetails> m_shader_details;
    PipelineLayoutBuilder* m_layout_builder;
    std::unique_ptr<PipelineLayoutBuilder> m_owned_builder;
    VkPipelineCache m_pipeline_cache = nullptr;

    ArenaAllocator m_arena;
};

class VertexInputDescriptionBuilder;

class GPipelineBuilder : public PipelineBuilderBase {
public:
    GPipelineBuilder(Core* core);

    void set_renderpass(Renderpass* renderpass, u32 subpass_index) {
        m_renderpass    = renderpass;
        m_subpass_index = subpass_index;
    };

    void set_topology(VkPrimitiveTopology topology);                                  // Set to triangle list by default
    void set_rasterization(VkPolygonMode polygon_mode, VkCullModeFlagBits cull_mode); // Set to Triangle Fill & No Cull
    void set_depth_testing(bool depth_testing);                                       // Defautls to true if renderpass has depth buffer else false
    inline void set_vertex_input(const VertexInputDescriptionBuilder* builder) { m_input_description_builder = builder; };

    std::unique_ptr<Pipeline> build();

private:
    void set_opaque_color_blend();

    const VertexInputDescriptionBuilder* m_input_description_builder = nullptr;
    Renderpass* m_renderpass                                         = nullptr;
    u32 m_subpass_index                                              = 0;
    bool default_depth                                               = true;

    VkPipelineDepthStencilStateCreateInfo m_depth_stencil                      = {};
    VkPipelineVertexInputStateCreateInfo m_vertex_input_info                   = {};
    VkPipelineInputAssemblyStateCreateInfo m_input_assembly                    = {};
    VkPipelineRasterizationStateCreateInfo m_rasterizer                        = {};
    VkPipelineMultisampleStateCreateInfo m_multisampling                       = {};
    std::vector<VkPipelineColorBlendAttachmentState> m_color_blend_attachments = {};
};

class CPipelineBuilder : public PipelineBuilderBase {
public:
    CPipelineBuilder(Core* core);

    std::unique_ptr<Pipeline> build();
};

} // namespace vke
