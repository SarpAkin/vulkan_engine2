#include "multi_pass_renderpass.hpp"

#include "commandbuffer.hpp"
#include "core.hpp"
#include "image.hpp"
#include "render_pass_builder.hpp"
#include "vkutil.hpp"
#include "window.hpp"

#include "../util.hpp"

namespace vke {

MultiPassRenderPass::MultiPassRenderPass(Core* core, RenderPassBuilder* builder, u32 width, u32 height) : Renderpass(core) {
    m_width  = width;
    m_height = height;

    m_clear_values     = std::move(builder->m_clear_values);
    m_attachment_infos = std::move(builder->m_attachment_infos);
    m_renderpass       = builder->m_renderpass;
    m_subpasses        = std::move(builder->m_subpass_details);

    for (auto& s : m_subpasses) {
        s.renderpass = this;
    }

    create_attachments();
    create_framebuffers();
}

void MultiPassRenderPass::create_attachments() {
    m_attachments = map_vec_indicies(m_attachment_infos, [this](const impl::AttachmentInfo& info, int i) {
        if (info.is_surface_attachment) return Attachment{};

        VkImageUsageFlags flags = is_depth_format(info.description.format) ? VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT : VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        if (info.is_sampled) {
            flags |= VK_IMAGE_USAGE_SAMPLED_BIT;
            m_sampled_attachments.push_back(i);
        }
        if (info.is_input_attachment) {
            flags |= VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT;
        }

        if (!info.name.empty()) {
            m_attachment_indicies[info.name] = i;
        }

        return Attachment{
            .image = core()->create_image(ImageArgs{
                .format      = info.description.format,
                .usage_flags = flags,
                .width       = width(),
                .height      = height(),
                .layers      = 1,
            }),
        };
    });
}

void MultiPassRenderPass::create_framebuffers() {
    auto attachment_views = MAP_VEC_ALLOCA(m_attachments, [](const Attachment& att) {
        return att.image != nullptr ? att.image->view() : nullptr;
    });

    VkFramebufferCreateInfo fb_info = {
        .sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
        .renderPass      = m_renderpass,
        .attachmentCount = static_cast<uint32_t>(attachment_views.size()),
        .pAttachments    = attachment_views.data(),
        .width           = m_width,
        .height          = m_height,
        .layers          = 1,
    };

    if (m_has_surface_attachment) {
        auto surface                = m_window->surface();
        auto& swapchain_image_views = surface->get_swapchain_image_views();

        m_framebuffers.resize(swapchain_image_views.size());

        for (u32 i; i < swapchain_image_views.size(); i++) {
            attachment_views[m_surface_attachment_index] = swapchain_image_views[i];
            VK_CHECK(vkCreateFramebuffer(device(), &fb_info, nullptr, &m_framebuffers[i]));
        }
    } else {
        m_framebuffers.resize(1);
        VK_CHECK(vkCreateFramebuffer(device(), &fb_info, nullptr, &m_framebuffers[0]));
    }
}

void MultiPassRenderPass::destroy_framebuffers() {
    for (auto& fb : m_framebuffers) {
        vkDestroyFramebuffer(device(), fb, nullptr);
        fb = nullptr;
    }
    m_framebuffers.resize(0);
}

VkFramebuffer MultiPassRenderPass::next_framebuffer() {
    if (m_window) return m_framebuffers[m_window->surface()->get_swapchain_image_index()];

    return m_framebuffers[0];
}

MultiPassRenderPass::~MultiPassRenderPass() {
    destroy_framebuffers();
}

void MultiPassRenderPass::begin(CommandBuffer& cmd) {
    if (m_has_surface_attachment) {
        m_window->surface()->prepare();
    }

    Renderpass::begin(cmd);
}

vke::Image* MultiPassRenderPass::get_attachment_image(const char* attachment_name) {
    if (auto it = m_attachment_indicies.find(attachment_name); it != m_attachment_indicies.end()) {
        return m_attachments[it->second].image.get();
    } else {
        return nullptr;
    }
}

void MultiPassRenderPass::barrier_sampled_attachments(CommandBuffer& cmd) {
    if (m_sampled_attachments.empty()) return;

    auto barriers = MAP_VEC_ALLOCA(m_sampled_attachments, [&](u32 index) {
        auto& attachment = m_attachments[index];

        return VkImageMemoryBarrier{
            .sType            = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .srcAccessMask    = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
            .dstAccessMask    = VK_ACCESS_SHADER_READ_BIT,
            .oldLayout        = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            .newLayout        = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            .image            = attachment.image->handle(),
            .subresourceRange = VkImageSubresourceRange{
                .aspectMask     = is_depth_format(attachment.image->format()) ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel   = 0,
                .levelCount     = 1,
                .baseArrayLayer = 0,
                .layerCount     = 1,
            },
        };
    });

    cmd.pipeline_barrier(PipelineBarrierArgs{
        .src_stage_mask        = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .dst_stage_mask        = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        .image_memory_barriers = barriers,
    });
}
void MultiPassRenderPass::end(CommandBuffer& cmd) {
    Renderpass::end(cmd);

    barrier_sampled_attachments(cmd);
}


void MultiPassRenderPass::resize(int width, int height) {
    m_width = width;
    m_height = height;

    destroy_framebuffers();
    create_attachments();
    create_framebuffers();
}
} // namespace vke