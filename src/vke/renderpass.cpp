#include "renderpass.hpp"

#include <vulkan/vulkan_core.h>

#include "commandbuffer.hpp"
#include "common.hpp"
#include "fwd.hpp"
#include "util.hpp"
#include "vkutil.hpp"
#include "window.hpp"

namespace vke {
void Renderpass::begin(CommandBuffer& cmd) {
    VkRenderPassBeginInfo rp_begin_info{
        .sType       = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .renderPass  = m_renderpass,
        .framebuffer = next_framebuffer(),
        .renderArea  = {
             .offset = {0, 0},
             .extent = {m_width, m_height},
        },
        .clearValueCount = static_cast<uint32_t>(m_clear_values.size()),
        .pClearValues    = m_clear_values.data(),
    };

    cmd.cmd_begin_renderpass(&rp_begin_info, VK_SUBPASS_CONTENTS_INLINE);

    VkViewport view_port = {
        .x        = 0.f,
        .y        = 0.f,
        .width    = static_cast<float>(m_width),
        .height   = static_cast<float>(m_height),
        .minDepth = 0.f,
        .maxDepth = 1.f,
    };

    vkCmdSetViewport(cmd.handle(), 0, 1, &view_port);

    VkRect2D scissor = {
        .offset = {0, 0},
        .extent = {m_width, m_height},
    };

    vkCmdSetScissor(cmd.handle(), 0, 1, &scissor);
}

void CommandBuffer::begin_renderpass(Renderpass* renderpass, VkSubpassContents contents) {
    renderpass->begin(*this);
}

void Renderpass::next_subpass(CommandBuffer& cmd) {
    cmd.cmd_next_subpass(VK_SUBPASS_CONTENTS_INLINE);
}

void Renderpass::end(CommandBuffer& cmd) {
    cmd.cmd_end_renderpass();
}

Renderpass::~Renderpass() {
    vkDestroyRenderPass(device(), m_renderpass, nullptr);
}

WindowRenderPass::WindowRenderPass(Window* window) : Renderpass(window->surface()->core()) {
    m_width  = window->width();
    m_height = window->height();
    m_window = window;

    m_clear_values = {VkClearValue{.color = VkClearColorValue{0.f, 0.f, 1.f, 0.f}}};

    init_renderpass();
    create_framebuffers();

    m_subpasses.push_back(SubpassDetails{
        .color_attachments = {m_window->surface()->get_swapchain_image_format()},
        .renderpass        = this,
        .subpass_index     = 0,
    });
}

void WindowRenderPass::init_renderpass() {
    auto surface = m_window->surface();

    VkAttachmentDescription attachments[] = {
        VkAttachmentDescription{
            .format        = surface->get_swapchain_image_format(),
            .samples       = VK_SAMPLE_COUNT_1_BIT,
            .loadOp        = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp       = VK_ATTACHMENT_STORE_OP_STORE,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .finalLayout   = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
        },
    };

    VkAttachmentReference attachment_references[] = {
        VkAttachmentReference{
            .attachment = 0,
            .layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        },
    };

    VkSubpassDescription subpasses[] = {
        VkSubpassDescription{
            .pipelineBindPoint    = VK_PIPELINE_BIND_POINT_GRAPHICS,
            .colorAttachmentCount = ARRAY_LEN(attachment_references),
            .pColorAttachments    = attachment_references,
        },
    };

    VkRenderPassCreateInfo info{
        .sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .attachmentCount = ARRAY_LEN(attachments),
        .pAttachments    = attachments,
        .subpassCount    = ARRAY_LEN(subpasses),
        .pSubpasses      = subpasses,
    };

    VK_CHECK(vkCreateRenderPass(device(), &info, nullptr, &m_renderpass));
}

void WindowRenderPass::create_framebuffers() {
    auto surface                = m_window->surface();
    auto& swapchain_image_views = surface->get_swapchain_image_views();

    VkImageView attachment_views[] = {nullptr};

    m_framebuffers.resize(swapchain_image_views.size());

    for (int i = 0; i < swapchain_image_views.size(); ++i) {
        attachment_views[0] = swapchain_image_views[i];

        VkFramebufferCreateInfo fb_info = {
            .sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            .renderPass      = m_renderpass,
            .attachmentCount = static_cast<u32>(ARRAY_LEN(attachment_views)),
            .pAttachments    = attachment_views,
            .width           = width(),
            .height          = height(),
            .layers          = 1,
        };

        VK_CHECK(vkCreateFramebuffer(device(), &fb_info, nullptr, &m_framebuffers[i]));
    }
}

void WindowRenderPass::destroy_framebuffers() {
    for (auto& fb : m_framebuffers) {
        vkDestroyFramebuffer(device(), fb, nullptr);
        fb = nullptr;
    }
    m_framebuffers.resize(0);
}

VkFramebuffer WindowRenderPass::next_framebuffer() {
    return m_framebuffers[m_window->surface()->get_swapchain_image_index()];
}

WindowRenderPass::~WindowRenderPass() {
    destroy_framebuffers();
}

void WindowRenderPass::begin(CommandBuffer& cmd) {
    m_window->surface()->prepare();

    Renderpass::begin(cmd);
}

} // namespace vke