#include "hierarchical_z_buffers.hpp"

#include <vke/pipeline_loader.hpp>
#include <vke/vke.hpp>
#include <vke/vke_builders.hpp>

#include "glm/ext/vector_float2.hpp"
#include "render/render_server.hpp"

vke::HierarchicalZBuffers::HierarchicalZBuffers(RenderServer* rd, IImageView* target) {
    m_render_server       = rd;
    m_target_depth_buffer = target;

    u32 width  = target->width() / 2;
    u32 height = target->height() / 2;

    u32 mip_layer_count = static_cast<u32>(std::ceil(std::log2(std::max(width, height))));

    m_depth_chain = std::make_unique<vke::Image>(ImageArgs{
        .format      = target->format(),
        .usage_flags = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT,
        .width       = width,
        .height      = height,
        .mip_levels  = mip_layer_count,
    });

    m_are_images_new = true;

    get_or_create_shared_data();

    create_sets();
}

void vke::HierarchicalZBuffers::get_or_create_shared_data() {
    auto& any_storage = m_render_server->get_any_storage();

    const std::string key = "vke::HZB::shared_data";
    if (auto d = at(any_storage, key)) {
        auto data = std::any_cast<std::weak_ptr<SharedData>>(d.value());

        auto locked = data.lock();
        if (locked) {
            m_shared_data = locked;
            return;
        }
    }

    VkSampler sampler;

    VkSamplerCreateInfo sampler_info{
        .sType        = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
        .magFilter    = VK_FILTER_LINEAR,
        .minFilter    = VK_FILTER_LINEAR,
        .addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
        .addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
        .addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
    };

    VkSamplerReductionModeCreateInfoEXT create_info_reduction = {
        .sType         = VK_STRUCTURE_TYPE_SAMPLER_REDUCTION_MODE_CREATE_INFO_EXT,
        .reductionMode = VK_SAMPLER_REDUCTION_MODE_MAX, // we use max as we use reversed z buffering
    };

    sampler_info.pNext = &create_info_reduction;

    VK_CHECK(vkCreateSampler(m_render_server->get_device(), &sampler_info, nullptr, &sampler));

    vke::DescriptorSetLayoutBuilder builder;
    builder.add_image_sampler(VK_SHADER_STAGE_COMPUTE_BIT);
    builder.add_binding(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT, 1);
    VkDescriptorSetLayout set_layout = builder.build();

    m_render_server->get_pipeline_loader()->get_pipeline_globals_provider()->set_layouts["vke::depth_mip_chain_set"] = set_layout;

    m_shared_data = std::make_shared<SharedData>(SharedData{
        .mip_pipeline         = m_render_server->get_pipeline_loader()->load("vke::depth_mip_pipeline"),
        .min_sampler          = sampler,
        .mip_chain_set_layout = set_layout,
    });

    any_storage[key] = std::weak_ptr(m_shared_data);
}

void vke::HierarchicalZBuffers::create_sets() {
    auto sampler = m_shared_data->min_sampler;

    for (int i = 0; i < m_depth_chain->miplevel_count(); i++) {
        auto view = m_depth_chain->create_subview(SubViewArgs{
            .base_miplevel  = static_cast<u32>(i),
            .miplevel_count = 1,
            .view_type      = VK_IMAGE_VIEW_TYPE_2D,
        });

        vke::DescriptorSetBuilder builder;
        builder.add_image_sampler(i == 0 ? m_target_depth_buffer : m_mip_views.back().get(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, sampler, VK_SHADER_STAGE_COMPUTE_BIT);
        builder.add_storage_image(view.get(), VK_IMAGE_LAYOUT_GENERAL, VK_SHADER_STAGE_COMPUTE_BIT);
        m_mip_sets.push_back(builder.build(m_render_server->get_descriptor_pool(), m_shared_data->mip_chain_set_layout));

        m_mip_views.push_back(std::move(view));
    }
}

void vke::HierarchicalZBuffers::update_mips(vke::CommandBuffer& cmd) {
    const glm::vec2 subgroup_size = {16, 16};

    assert(m_mip_sets.size() == m_mip_views.size());

    cmd.bind_pipeline(m_shared_data->mip_pipeline.get());

    VkImageMemoryBarrier barriers[] = {
        VkImageMemoryBarrier{
            .sType            = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .srcAccessMask    = VK_ACCESS_MEMORY_READ_BIT,
            .dstAccessMask    = VK_ACCESS_MEMORY_WRITE_BIT,
            .oldLayout        = m_are_images_new ? VK_IMAGE_LAYOUT_UNDEFINED : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            .newLayout        = VK_IMAGE_LAYOUT_GENERAL,
            .image            = m_depth_chain->handle(),
            .subresourceRange = m_depth_chain->get_subresource_range(),
        },
    };

    cmd.pipeline_barrier(PipelineBarrierArgs{
        .src_stage_mask        = m_are_images_new ? static_cast<VkPipelineStageFlags>(VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT) : (VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT),
        .dst_stage_mask        = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
        .image_memory_barriers = barriers,
    });

    for (int i = 0; i < m_mip_sets.size(); i++) {
        cmd.bind_descriptor_set(0, m_mip_sets[i]);

        auto target_view = m_mip_views[i].get();

        auto [ex, ey] = target_view->extend();

        auto [sx, sy] = vke::calculate_dispatch_size(ex, ey, subgroup_size.x, subgroup_size.y);

        cmd.dispatch(sx, sy, 1);

        VkImageMemoryBarrier barriers[] = {
            VkImageMemoryBarrier{
                .sType            = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
                .srcAccessMask    = VK_ACCESS_MEMORY_WRITE_BIT,
                .dstAccessMask    = VK_ACCESS_MEMORY_READ_BIT,
                .oldLayout        = VK_IMAGE_LAYOUT_GENERAL,
                .newLayout        = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                .image            = target_view->vke_image()->handle(),
                .subresourceRange = target_view->get_subresource_range(),
            },
        };

        cmd.pipeline_barrier(PipelineBarrierArgs{
            .src_stage_mask        = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
            .dst_stage_mask        = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            .image_memory_barriers = barriers,
        });
    }
}
