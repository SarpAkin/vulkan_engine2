#pragma once

#include <memory>
#include <optional>
#include <vector>
#include <vulkan/vulkan_core.h>

#include "../common.hpp"
#include "../fwd.hpp"
#include "semaphore.hpp"
#include "vk_resource.hpp"

namespace vke {

struct SubpassDetails {
    std::vector<VkFormat> color_attachments;
    std::optional<VkFormat> depth_format;
    Renderpass* renderpass;
    u32 subpass_index;
};

class Renderpass : public Resource {
public: // getters
    inline u32 width() const { return m_width; }
    inline u32 height() const { return m_height; }
    inline VkRenderPass handle() const { return m_renderpass; }
    inline const SubpassDetails* get_subpass(usize subpass_index) const { return &m_subpasses[subpass_index]; }

public: // methods
    virtual void set_states(CommandBuffer& cmd);

    virtual vke::IImageView* get_attachment_image_view(const char* attachment_name) = 0;

    virtual void resize(int width, int height) { abort(); }

    virtual void begin(CommandBuffer& cmd);
    virtual void next_subpass(CommandBuffer& cmd);
    virtual void end(CommandBuffer& cmd);
    virtual void set_external(bool is_external) { m_is_external = is_external; }
    virtual bool has_depth(u32 subpass) { return get_subpass(subpass)->depth_format.has_value(); }

    void set_resize_event_manager(EventManager<int, int>* event_man);

    Renderpass(Core* core) : Resource(core) {}
    ~Renderpass();

protected:
    virtual VkFramebuffer next_framebuffer() = 0;

    EventManager<int, int>* m_resize_eman = nullptr;

protected:
    bool m_is_external;
    u32 m_width, m_height;
    VkRenderPass m_renderpass = nullptr; // should be destroyed by this class, created by child class.
    std::vector<VkClearValue> m_clear_values;
    std::vector<SubpassDetails> m_subpasses; // shouldn't be modified after creation. especially should't be resized since pointers to elements might be created.
};

class WindowRenderPass : public Renderpass {
public:
    WindowRenderPass(Window* window, bool include_depth_buffer = true);
    ~WindowRenderPass();

    void resize(int, int) override;

    void begin(CommandBuffer& cmd) override;

    bool has_depth(u32 subpass) override { return m_depth != nullptr; }

    vke::IImageView* get_attachment_image_view(const char* attachment_name) override { return nullptr; }

private:
    void create_depth_image();
    void init_renderpass();
    void create_framebuffers();
    void destroy_framebuffers();

    VkFramebuffer next_framebuffer() override;

private:
    Window* m_window = nullptr;
    std::vector<VkFramebuffer> m_framebuffers;
    std::unique_ptr<vke::Image> m_depth;
};

} // namespace vke