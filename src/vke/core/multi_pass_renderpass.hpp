#pragma once

#include <memory>
#include <unordered_map>
#include <vector>

#include "../fwd.hpp"
#include "renderpass.hpp"

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
    void end(CommandBuffer& cmd) override;

    void resize(int, int) override;

private:
    void create_attachments();
    void create_framebuffers();
    void destroy_framebuffers();

    void barrier_sampled_attachments(CommandBuffer& cmd);

private:
    struct Attachment {
        std::unique_ptr<Image> image;
    };

    Window* m_window               = nullptr;
    bool m_has_surface_attachment  = false;
    u32 m_surface_attachment_index = UINT32_MAX;

    std::vector<VkFramebuffer> m_framebuffers;
    std::vector<impl::AttachmentInfo> m_attachment_infos;

    std::vector<Attachment> m_attachments;
    std::vector<u32> m_sampled_attachments;

    std::unordered_map<std::string, u32> m_attachment_indicies;
};
} // namespace vke
