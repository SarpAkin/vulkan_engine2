#pragma once

#include <memory>
#include <optional>
#include <vector>

#include "../common.hpp"
#include "../fwd.hpp"
#include "renderpass.hpp"

namespace vke {

namespace impl {
struct AttachmentInfo {
    VkAttachmentDescription description;
    std::string name;
    bool is_sampled;
    bool is_input_attachment   = false;
    bool is_surface_attachment = false;
    std::shared_ptr<vke::IImageView> view;
};

struct SubpassInfo {
    std::vector<VkAttachmentReference> color_attachments;
    std::optional<VkAttachmentReference> depth_attachment;
    std::vector<VkAttachmentReference> input_attachments;
};
} // namespace impl

const VkClearValue DEPTH_CLEAR_VALUE{.depthStencil = {.depth = 1.0}};

class MultiPassRenderPass;

class RenderPassBuilder {
    friend MultiPassRenderPass;

public:
    u32 add_attachment(VkFormat format, std::optional<VkClearValue> clear_value, bool is_sampled = false, const char* name = nullptr);
    u32 add_attachment(std::unique_ptr<vke::IImageView> view, std::optional<VkClearValue> clear_value, bool is_sampled = false, const char* name = nullptr);

    void add_subpass(const std::initializer_list<u32>& color_attachments, const std::optional<u32>& depth_attachment, const std::initializer_list<u32>& input_attachments);

    std::unique_ptr<Renderpass> build(Core* core, u32 width, u32 height);

private:
    std::vector<VkSubpassDependency> create_subpass_dependencies();
    VkRenderPass create_vk_renderpass(Core* core);

    std::vector<impl::AttachmentInfo> m_attachment_infos;
    std::vector<VkClearValue> m_clear_values;
    std::vector<impl::SubpassInfo> m_subpass_info;

    VkRenderPass m_renderpass;
    std::vector<SubpassDetails> m_subpass_details;
};
} // namespace vke