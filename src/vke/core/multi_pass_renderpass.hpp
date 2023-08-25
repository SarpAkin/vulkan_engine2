#pragma once

#include <memory>
#include <unordered_map>
#include <vector>

#include "renderpass.hpp"
#include "../fwd.hpp"


namespace vke {
class RenderPassBuilder;

namespace impl {
    struct AttachmentInfo;
}

class MultiPassRenderPass : public Renderpass {
    friend RenderPassBuilder;

public:
    MultiPassRenderPass(Core* core, RenderPassBuilder* builder, u32 width, u32 height);
    ~MultiPassRenderPass();

    vke::Image* get_attachment_image(const char* attachment_name) override;

    VkFramebuffer next_framebuffer() override;

    void begin(CommandBuffer& cmd) override;
private:
    void create_attachments();
    void create_framebuffers();
    void destroy_framebuffers();

private:
    struct Attachment{
        std::unique_ptr<Image> image;
    };

    Window* m_window = nullptr;
    bool m_has_surface_attachment = false;
    u32 m_surface_attachment_index = UINT32_MAX;

    std::vector<VkFramebuffer> m_framebuffers;
    std::vector<impl::AttachmentInfo> m_attachment_infos;

    std::vector<Attachment> m_attachments;

    std::unordered_map<std::string, u32> m_attachment_indicies;
};
} // namespace vke
