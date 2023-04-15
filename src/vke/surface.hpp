#pragma once

#include <cstdint>
#include <memory>
#include <vector>

#include <vulkan/vulkan_core.h>

#include "common.hpp"
#include "fwd.hpp"
#include "semaphore.hpp"
#include "vk_resource.hpp"

namespace vke {

class Surface : public Resource {
public: // getters
    VkSurfaceKHR get_surface() const { return m_surface; }
    VkFormat get_swapchain_image_format() const { return m_swapchain_image_format; }
    VkSwapchainKHR get_swapchain() const { return m_swapchain; }
    const std::vector<VkImage>& get_swapchain_images() const { return m_swapchain_images; }
    const std::vector<VkImageView>& get_swapchain_image_views() const { return m_swapchain_image_views; }
    VkAttachmentDescription get_color_attachment() const;

    Surface(Core* core, VkSurfaceKHR surface) : Resource(core) {
        m_surface = surface;
    }
    ~Surface();

    // must be called after device initialization
    void init_swapchain();

    void prepare(u64 time_out = UINT64_MAX);
    void present();

    u32 get_swapchain_image_index() const { return m_swapchain_image_index; }

    Semaphore* get_prepare_semaphore()const {return m_current_prepare_semaphore;} //returns null if prepare isn't called that frame
    Semaphore* get_wait_semaphore() const {return m_current_wait_semaphore;}
private:
    VkSurfaceKHR m_surface;
    VkFormat m_swapchain_image_format;
    VkSwapchainKHR m_swapchain = nullptr;
    std::vector<VkImage> m_swapchain_images;
    std::vector<VkImageView> m_swapchain_image_views;

    u32 m_swapchain_image_index = 0;

    //prepare semaphore is signalled after surface preparetion, wait semapore is used to wait for present
    std::vector<std::unique_ptr<Semaphore>> m_prepare_semaphores, m_wait_semaphores;

    Semaphore* m_current_prepare_semaphore = nullptr;
    Semaphore* m_current_wait_semaphore = nullptr;
};

} // namespace vke