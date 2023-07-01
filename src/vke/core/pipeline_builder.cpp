#include "pipeline_builder.hpp"

#include <cassert>
#include <filesystem>
#include <initializer_list>
#include <memory>
#include <vulkan/vulkan_core.h>

#include <spirv_reflect.h>

#include "descriptor_set_layout_builder.hpp"
#include "pipeline.hpp"
#include "renderpass.hpp"
#include "vertex_input_builder.hpp"
#include "vkutil.hpp"

#include "../util.hpp"

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

void PipelineBuilderBase::add_shader_stage(u32* spirv_code, usize spirv_len, VkShaderStageFlagBits stage) {

    VkShaderModule module = nullptr;
    if (stage != 0) {
        VkShaderModuleCreateInfo c_info{
            .sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
            .codeSize = spirv_len * 4, // convert to byte size
            .pCode    = spirv_code,
        };

        VK_CHECK(vkCreateShaderModule(device(), &c_info, nullptr, &module));
    }

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

    if(default_depth){
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
    auto layouts = build_layout_and_shaders();

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
    return vke_pipeline;
}

#define SPV_CHECK(x)                                                    \
    {                                                                   \
        SpvReflectResult result = x;                                    \
        if (result != SPV_REFLECT_RESULT_SUCCESS) {                     \
            fmt::print(stderr, "[SPV Reflection Error]: {}\n", result); \
            assert(0);                                                  \
        }                                                               \
    }

VkShaderStageFlags convert_to_vk(SpvReflectShaderStageFlagBits stage) {
    switch (stage) {
    case SPV_REFLECT_SHADER_STAGE_VERTEX_BIT:
        return VK_SHADER_STAGE_VERTEX_BIT;
    case SPV_REFLECT_SHADER_STAGE_TESSELLATION_CONTROL_BIT:
        return VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
    case SPV_REFLECT_SHADER_STAGE_TESSELLATION_EVALUATION_BIT:
        return VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
    case SPV_REFLECT_SHADER_STAGE_GEOMETRY_BIT:
        return VK_SHADER_STAGE_GEOMETRY_BIT;
    case SPV_REFLECT_SHADER_STAGE_FRAGMENT_BIT:
        return VK_SHADER_STAGE_FRAGMENT_BIT;
    case SPV_REFLECT_SHADER_STAGE_COMPUTE_BIT:
        return VK_SHADER_STAGE_COMPUTE_BIT;
    case SPV_REFLECT_SHADER_STAGE_TASK_BIT_NV:
        return VK_SHADER_STAGE_TASK_BIT_NV;
    case SPV_REFLECT_SHADER_STAGE_MESH_BIT_NV:
        return VK_SHADER_STAGE_MESH_BIT_NV;
    case SPV_REFLECT_SHADER_STAGE_RAYGEN_BIT_KHR:
        return VK_SHADER_STAGE_RAYGEN_BIT_KHR;
    case SPV_REFLECT_SHADER_STAGE_ANY_HIT_BIT_KHR:
        return VK_SHADER_STAGE_ANY_HIT_BIT_KHR;
    case SPV_REFLECT_SHADER_STAGE_CLOSEST_HIT_BIT_KHR:
        return VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
    case SPV_REFLECT_SHADER_STAGE_MISS_BIT_KHR:
        return VK_SHADER_STAGE_MISS_BIT_KHR;
    case SPV_REFLECT_SHADER_STAGE_INTERSECTION_BIT_KHR:
        return VK_SHADER_STAGE_INTERSECTION_BIT_KHR;
    case SPV_REFLECT_SHADER_STAGE_CALLABLE_BIT_KHR:
        return VK_SHADER_STAGE_CALLABLE_BIT_KHR;
    }

    assert(0);
    return 0;
}

VkDescriptorType convert_to_vk(SpvReflectDescriptorType type) {
    switch (type) {
    case SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLER:
        return VK_DESCRIPTOR_TYPE_SAMPLER;
    case SPV_REFLECT_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
        return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    case SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
        return VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
    case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_IMAGE:
        return VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
        return VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
    case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:
        return VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
    case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
        return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER:
        return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
        return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
        return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;
    case SPV_REFLECT_DESCRIPTOR_TYPE_INPUT_ATTACHMENT:
        return VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
    case SPV_REFLECT_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR:
        return VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
    }

    assert(0);
    return VK_DESCRIPTOR_TYPE_MAX_ENUM;
}

PipelineBuilderBase::LayoutBuild PipelineBuilderBase::build_layout_and_shaders() {
    struct DescriptorSetInfo {
        struct BindingInfo {
            VkDescriptorType type;
            VkShaderStageFlags stage;
            u32 count;
        };
        std::span<BindingInfo> bindings;
    } set_infos[4] = {};

    u32 push_size = UINT32_MAX;
    VkShaderStageFlags push_stage;

    for (auto& shader : m_shader_details) {

        SpvReflectShaderModule module;
        SPV_CHECK(spvReflectCreateShaderModule(shader.spirv_len * 4, shader.spirv_code, &module));

        VkShaderStageFlags stage = convert_to_vk(module.shader_stage);
        if (shader.module == nullptr) {
            shader.stage = (VkShaderStageFlagBits)stage;

            VkShaderModuleCreateInfo c_info{
                .sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
                .codeSize = shader.spirv_len * 4, // convert to byte size
                .pCode    = shader.spirv_code,
            };

            VK_CHECK(vkCreateShaderModule(device(), &c_info, nullptr, &shader.module));
        }

        u32 var_count = 0;
        SPV_CHECK(spvReflectEnumerateDescriptorSets(&module, &var_count, nullptr));

        std::span<SpvReflectDescriptorSet*> dsets = ALLOCA_ARR(SpvReflectDescriptorSet*, var_count);
        SPV_CHECK(spvReflectEnumerateDescriptorSets(&module, &var_count, dsets.data()));

        for (auto& set : dsets) {
            auto& set_info = set_infos[set->set];
            if (set_info.bindings.data() == nullptr) {
                set_info.bindings = ALLOCA_ARR(DescriptorSetInfo::BindingInfo, set->binding_count);
                memset(set_info.bindings.data(), 0, set_info.bindings.size_bytes());
            }

            for (int i = 0; i < set->binding_count; ++i) {
                auto& reflection_binding = set->bindings[i];

                auto& binding_info = set_info.bindings[reflection_binding->binding];

                binding_info.type = convert_to_vk(reflection_binding->descriptor_type);
                binding_info.stage |= stage;
                binding_info.count = reflection_binding->count;
            }
        }

        u32 push_count;
        SPV_CHECK(spvReflectEnumeratePushConstantBlocks(&module, &push_count, nullptr));
        std::span<SpvReflectBlockVariable*> push_blocks = ALLOCA_ARR(SpvReflectBlockVariable*, push_count);
        SPV_CHECK(spvReflectEnumeratePushConstantBlocks(&module, &push_count, push_blocks.data()));

        if (push_size == UINT32_MAX && push_count > 0) {
            push_size  = push_blocks[0]->size;
            push_stage = stage;
        }

        spvReflectDestroyShaderModule(&module);
    }

    std::vector<VkDescriptorSetLayout> dset_layouts;

    for (auto& set_info : std::span(set_infos)) {
        if (set_info.bindings.empty()) {
            break;
            // dset_layouts.push_back(nullptr);
        }

        DescriptorSetLayoutBuilder builder;
        for (auto& binding : set_info.bindings) {
            builder.add_binding(binding.type, binding.stage, binding.count);
        }

        dset_layouts.push_back(builder.build(core()));
    }

    PipelineLayoutBuilder p_builder;
    if (push_size != UINT32_MAX) {
        p_builder.add_push_constant(push_stage, push_size);
    }

    for (auto& set_layout : dset_layouts) {
        p_builder.add_set_layout(set_layout);
    }

    VkPipelineLayout layout = p_builder.build(device());

    dset_layouts.resize(4);

    return LayoutBuild{
        .layout       = layout,
        .dset_layouts = dset_layouts,
        .push_stages  = (VkShaderStageFlagBits)push_stage,
    };
}

CPipelineBuilder::CPipelineBuilder(Core* core) : PipelineBuilderBase(core) {}

std::unique_ptr<Pipeline> CPipelineBuilder::build() {
    assert(m_shader_details.size() == 1 && "compute pipeline must have exactly one shader module");

    auto layout_details = build_layout_and_shaders();

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

    return vke_pipeline;
}

void PipelineBuilderBase::add_shader_stage(std::string_view spirv_path) {
    std::filesystem::path p = spirv_path;

    if (p.extension() != ".spv") {
        auto binary = compile_glsl_file(&m_arena, spirv_path.begin());
        this->add_shader_stage(binary);

    } else {
        auto binary = read_file_binary(&m_arena,spirv_path.begin());
        this->add_shader_stage(binary);
    }
}

std::optional<BufferRefletion> reflect_binding(SpvReflectDescriptorBinding* binding) {
    if (binding->descriptor_type != SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER && binding->descriptor_type != SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER) return std::nullopt;
    auto* block = &binding->block;

    return BufferRefletion(block);
}

std::optional<BufferRefletion> PipelineBuilderBase::reflect_buffer(u32 nset, u32 nbinding) {
    for (auto& shader : m_shader_details) {
        SpvReflectShaderModule module;
        SPV_CHECK(spvReflectCreateShaderModule(shader.spirv_len * 4, shader.spirv_code, &module));

        u32 var_count = 0;
        SPV_CHECK(spvReflectEnumerateDescriptorBindings(&module, &var_count, nullptr));
        std::span<SpvReflectDescriptorBinding*> bindings = ALLOCA_ARR(SpvReflectDescriptorBinding*, var_count);
        SPV_CHECK(spvReflectEnumerateDescriptorBindings(&module, &var_count, bindings.data()));

        for (auto binding : bindings) {
            if (binding->set != nset || binding->binding != nbinding) continue;

            return reflect_binding(binding);
        }
    }

    return std::nullopt;
}

BufferRefletion::BufferRefletion(SpvReflectBlockVariable* block) {
    m_buffer_size = block->size;

    for (int i = 0; i < block->member_count; i++) {
        auto& member = block->members[i];
        using Type   = BufferRefletion::Field::Type;
        Type type;
        switch (member.type_description->type_flags) {
        case SPV_REFLECT_TYPE_FLAG_BOOL:
            type = Type::BOOL;
            break;
        case SPV_REFLECT_TYPE_FLAG_INT:
            type = Type::INT;
            break;
        case SPV_REFLECT_TYPE_FLAG_FLOAT:
            type = Type::FLOAT;
            break;
        case SPV_REFLECT_TYPE_FLAG_VECTOR:
            type = Type(Type::VEC_BASE + member.type_description->traits.numeric.vector.component_count);
            break;
        }

        m_fields.emplace(member.name, Field{
                                          .type   = type,
                                          .offset = member.absolute_offset,
                                      });
    }
}
} // namespace vke