#pragma once

#include <cstddef>
#include <memory>
#include <span>
#include <vector>
#include <vulkan/vulkan_core.h>

#include "common.hpp"
#include "fwd.hpp"
#include "pipeline.hpp"
#include "vk_resource.hpp"

namespace vke {

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
    PipelineBuilderBase(Core* core) : Resource(core) {}
    ~PipelineBuilderBase();

    // 0 is for auto
    void add_shader_stage(u32* spirv_code, usize spirv_len, VkShaderStageFlagBits stage = (VkShaderStageFlagBits)0);
    inline void add_shader_stage(std::span<u32> span, VkShaderStageFlagBits stage = (VkShaderStageFlagBits)0) { add_shader_stage(span.data(), span.size(), stage); };
    void set_layout_builder(PipelineLayoutBuilder* builder) { m_layout_builder = builder; }

protected:
    struct LayoutBuild {
        VkPipelineLayout layout;
        std::vector<VkDescriptorSetLayout> dset_layouts;
    };

    LayoutBuild build_layout_and_shaders();

    struct ShaderDetails {
        u32* spirv_code;
        u32 spirv_len;
        VkShaderStageFlagBits stage;
        VkShaderModule module;
    };

    std::vector<ShaderDetails> m_shader_details;
    PipelineLayoutBuilder* m_layout_builder;
    std::unique_ptr<PipelineLayoutBuilder> m_owned_builder;
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
    void set_rasterization(VkPolygonMode polygon_mode, VkCullModeFlagBits cull_mode); // Set to Triangle Fill & Back Cull
    void set_depth_testing(bool depth_testing);                                       // Set to false
    inline void set_vertex_input(VertexInputDescriptionBuilder* builder) { m_input_description_builder = builder; };

    std::unique_ptr<Pipeline> build();

private:
    void set_opaque_color_blend();

    VertexInputDescriptionBuilder* m_input_description_builder;
    Renderpass* m_renderpass = nullptr;
    u32 m_subpass_index      = 0;

    VkPipelineDepthStencilStateCreateInfo m_depth_stencil                      = {};
    VkPipelineVertexInputStateCreateInfo m_vertex_input_info                   = {};
    VkPipelineInputAssemblyStateCreateInfo m_input_assembly                    = {};
    VkPipelineRasterizationStateCreateInfo m_rasterizer                        = {};
    VkPipelineMultisampleStateCreateInfo m_multisampling                       = {};
    std::vector<VkPipelineColorBlendAttachmentState> m_color_blend_attachments = {};
};

} // namespace vke
