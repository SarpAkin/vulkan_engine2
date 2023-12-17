#include "pipeline_builder.hpp"

#include <cassert>
#include <filesystem>
#include <initializer_list>
#include <memory>
#include <vulkan/vulkan_core.h>

#include <spirv_reflect.h>

#include "descriptor_set_layout_builder.hpp"
#include "pipeline.hpp"
#include "pipeline_reflection.hpp"
#include "renderpass.hpp"
#include "vertex_input_builder.hpp"
#include "vkutil.hpp"

#include "../util.hpp"

#include "spv_reflect_util.hpp"

namespace vke {

PipelineBuilderBase::~PipelineBuilderBase() {
    for (auto& shader : m_shader_details) {
        vkDestroyShaderModule(device(), shader.module, nullptr);
    }
}

VkPipelineLayout PipelineLayoutBuilder::build(VkDevice device) {
    VkPipelineLayoutCreateInfo info = {
        .sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount         = static_cast<uint32_t>(m_set_layouts.size()),
        .pSetLayouts            = m_set_layouts.data(),
        .pushConstantRangeCount = static_cast<uint32_t>(m_push_constants.size()),
        .pPushConstantRanges    = m_push_constants.data(),
    };

    VkPipelineLayout layout;
    VK_CHECK(vkCreatePipelineLayout(device, &info, nullptr, &layout));
    return layout;
}

PipelineBuilderBase::PipelineBuilderBase(Core* core) : Resource(core) {
    m_reflection = std::make_unique<PipelineReflection>();
}

void PipelineBuilderBase::add_shader_stage(u32* spirv_code, usize spirv_len, VkShaderStageFlagBits stage) {

    VkShaderModule module = nullptr;

    auto reflected_stage = m_reflection->add_shader_stage(std::span(spirv_code, spirv_len));

    if (stage == 0) {
        stage = reflected_stage;
    }

    VkShaderModuleCreateInfo c_info{
        .sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = spirv_len * 4, // convert to byte size
        .pCode    = spirv_code,
    };

    VK_CHECK(vkCreateShaderModule(device(), &c_info, nullptr, &module));

    m_shader_details.push_back(ShaderDetails{
        .spirv_code = spirv_code,
        .spirv_len  = static_cast<u32>(spirv_len),
        .stage      = stage,
        .module     = module,
    });
}

void GPipelineBuilder::set_topology(VkPrimitiveTopology topology) {
    m_input_assembly = {
        .sType    = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .topology = topology,
    };
}

void GPipelineBuilder::set_rasterization(VkPolygonMode polygon_mode, VkCullModeFlagBits cull_mode) {
    m_rasterizer = {
        .sType       = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .polygonMode = polygon_mode,
        .cullMode    = cull_mode,
        .frontFace   = VK_FRONT_FACE_CLOCKWISE,
        .lineWidth   = 1.f,
    };
}

void GPipelineBuilder::set_depth_testing(bool depth_testing) {
    m_depth_stencil = {
        .sType            = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        .depthTestEnable  = depth_testing,
        .depthWriteEnable = depth_testing,
        .depthCompareOp   = depth_testing ? VK_COMPARE_OP_LESS_OR_EQUAL : VK_COMPARE_OP_ALWAYS,
    };
}

GPipelineBuilder::GPipelineBuilder(Core* core) : PipelineBuilderBase(core) {
    m_multisampling = {
        .sType                = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
        .minSampleShading     = 1.0f,
    };

    // color_blend_attachment = {
    //     .blendEnable    = false,
    //     .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
    // };

    // vertex_input_info = {
    //     .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
    // };

    set_topology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    set_rasterization(VK_POLYGON_MODE_FILL, VK_CULL_MODE_NONE);
    set_depth_testing(false);
}

std::unique_ptr<Pipeline> GPipelineBuilder::build() {
    assert(m_renderpass);

    if (default_depth) {
        set_depth_testing(m_renderpass->has_depth(m_subpass_index));
    }

    VkPipelineViewportStateCreateInfo viewport_state = {
        .sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .viewportCount = 1,
        .scissorCount  = 1,
    };

    auto subpass = m_renderpass->get_subpass(m_subpass_index);

    m_color_blend_attachments.resize(subpass->color_attachments.size(),
        VkPipelineColorBlendAttachmentState{
            .blendEnable    = false,
            .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
        });

    VkPipelineColorBlendStateCreateInfo color_blending = {
        .sType           = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .pNext           = nullptr,
        .logicOpEnable   = VK_FALSE,
        .logicOp         = VK_LOGIC_OP_COPY,
        .attachmentCount = static_cast<u32>(m_color_blend_attachments.size()),
        .pAttachments    = m_color_blend_attachments.data(),
    };

    VkDynamicState dynamic_states[] = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR,
    };

    VkPipelineDynamicStateCreateInfo dynamic_state_info = {
        .sType             = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .dynamicStateCount = static_cast<u32>(ARRAY_LEN(dynamic_states)),
        .pDynamicStates    = dynamic_states,
    };

    // must be called before creating shader stages since it can create shader modules if their stage is left to default
    auto layouts = m_reflection->build_pipeline_layout(core());

    auto shader_stages = MAP_VEC_ALLOCA(m_shader_details, [](const ShaderDetails& shader_detail) {
        return VkPipelineShaderStageCreateInfo{
            .sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage  = shader_detail.stage,
            .module = shader_detail.module,
            .pName  = "main",
        };
    });

    VkPipelineVertexInputStateCreateInfo vertex_input_info = {
        .sType                         = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .vertexBindingDescriptionCount = 0,
    };
    if (m_input_description_builder) {
        vertex_input_info = m_input_description_builder->get_info();
    }

    VkGraphicsPipelineCreateInfo pipeline_info = {
        .sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .stageCount          = static_cast<uint32_t>(shader_stages.size()),
        .pStages             = shader_stages.data(),
        .pVertexInputState   = &vertex_input_info,
        .pInputAssemblyState = &m_input_assembly,
        .pViewportState      = &viewport_state,
        .pRasterizationState = &m_rasterizer,
        .pMultisampleState   = &m_multisampling,
        .pDepthStencilState  = &m_depth_stencil,
        .pColorBlendState    = &color_blending,
        .pDynamicState       = &dynamic_state_info,
        .layout              = layouts.layout,
        .renderPass          = m_renderpass->handle(),
        .subpass             = m_subpass_index,
        .basePipelineHandle  = nullptr,
    };

    VkPipeline pipeline;
    VK_CHECK(vkCreateGraphicsPipelines(device(), m_pipeline_cache, 1, &pipeline_info, nullptr, &pipeline));

    auto vke_pipeline                 = std::make_unique<Pipeline>(core(), pipeline, layouts.layout, VK_PIPELINE_BIND_POINT_GRAPHICS);
    vke_pipeline->m_data.dset_layouts = std::move(layouts.dset_layouts);
    vke_pipeline->m_data.push_stages  = layouts.push_stages;
    vke_pipeline->m_reflection        = std::move(m_reflection);
    return vke_pipeline;
}

CPipelineBuilder::CPipelineBuilder(Core* core) : PipelineBuilderBase(core) {}

std::unique_ptr<Pipeline> CPipelineBuilder::build() {
    assert(m_shader_details.size() == 1 && "compute pipeline must have exactly one shader module");

    auto layout_details = m_reflection->build_pipeline_layout(core());

    VkComputePipelineCreateInfo create_info{
        .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
        .stage = VkPipelineShaderStageCreateInfo{
            .sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage  = VK_SHADER_STAGE_COMPUTE_BIT,
            .module = m_shader_details[0].module,
            .pName  = "main",
        },
        .layout = layout_details.layout,
    };

    VkPipeline vk_pipeline;
    vkCreateComputePipelines(device(), m_pipeline_cache, 1, &create_info, nullptr, &vk_pipeline);

    auto vke_pipeline                 = std::make_unique<Pipeline>(core(), vk_pipeline, layout_details.layout, VK_PIPELINE_BIND_POINT_COMPUTE);
    vke_pipeline->m_data.dset_layouts = std::move(layout_details.dset_layouts);
    vke_pipeline->m_data.push_stages  = layout_details.push_stages;
    vke_pipeline->m_reflection        = std::move(m_reflection);

    return vke_pipeline;
}

void PipelineBuilderBase::add_shader_stage(std::string_view spirv_path) {
    std::filesystem::path p = spirv_path;

    if (p.extension() != ".spv") {
        ShaderCompileOptions options{
            .defines = m_defines,
        };

        auto binary = compile_glsl_file(&m_arena, spirv_path.begin(), &options);
        this->add_shader_stage(binary);

    } else {
        auto binary = read_file_binary(&m_arena, spirv_path.begin());
        this->add_shader_stage(binary);
    }
}

} // namespace vke