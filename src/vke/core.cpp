#include "core.hpp"

#include "surface.hpp"
#include "vk_mem_alloc.h"
#include "vkutil.hpp"
#include "window.hpp"

#include "commandbuffer.hpp"
#include "fence.hpp"

#include <VkBootstrap.h>
#include <memory>
#include <vulkan/vulkan_core.h>

namespace vke {

struct Core::CoreData {
    vkb::Instance vkb_instance;
    vkb::PhysicalDevice vkb_pdevice;
    vkb::Device vkb_device;
};

Core::Core(CoreConfig* config) {
    auto vkb_instance =
        vkb::InstanceBuilder()
            .set_app_name(config->app_name)
            .require_api_version(config->vk_version_major, config->vk_version_minor, config->vk_version_patch)
#ifndef NDEBUG
            .request_validation_layers(true)
            .use_default_debug_messenger()
    // .set_debug_callback(debug_callback)
    // .set_debug_callback_user_data_pointer(m_data)
#endif
            .build()
            .value();

    m_instance = vkb_instance.instance;

    config->window->init_surface(this);

    vkb::PhysicalDeviceSelector selector{vkb_instance};
    selector.set_minimum_version(config->vk_version_major, config->vk_version_minor);
    if (config->window) {
        selector.set_surface(config->window->surface()->get_surface());
    }

    vkb::PhysicalDevice vkb_pdevice = selector.select().value();

    vkb::DeviceBuilder device_builder{vkb_pdevice};
    vkb::Device vkb_device = device_builder.build().value();

    // use vkbootstrap to get a Graphics queue
    m_chosen_gpu            = vkb_pdevice.physical_device;
    m_device                = vkb_device.device;
    m_graphics_queue        = vkb_device.get_queue(vkb::QueueType::graphics).value();
    m_graphics_queue_family = vkb_device.get_queue_index(vkb::QueueType::graphics).value();

    m_data               = std::make_unique<CoreData>();
    m_data->vkb_device   = vkb_device;
    m_data->vkb_pdevice  = vkb_pdevice;
    m_data->vkb_instance = vkb_instance;

    config->window->surface()->init_swapchain();

    init_allocator();
    init_managers();
}

void Core::init_allocator() {
    VmaVulkanFunctions vulkan_functions    = {};
    vulkan_functions.vkGetInstanceProcAddr = &vkGetInstanceProcAddr;
    vulkan_functions.vkGetDeviceProcAddr   = &vkGetDeviceProcAddr;

    VmaAllocatorCreateInfo allocator_create_info = {};
    allocator_create_info.vulkanApiVersion       = VK_API_VERSION_1_2;
    allocator_create_info.physicalDevice         = m_chosen_gpu;
    allocator_create_info.device                 = device();
    allocator_create_info.instance               = instance();
    allocator_create_info.pVulkanFunctions       = &vulkan_functions;

    VK_CHECK(vmaCreateAllocator(&allocator_create_info, &m_gpu_allocator));
}

void Core::init_managers() {
    m_sampler_manager = new SamplerManager(this);
}

Core::~Core() {
    delete m_sampler_manager;

    destroy_queued_handles();

    vmaDestroyAllocator(m_gpu_allocator);

    vkDestroyDevice(m_device, nullptr);
    vkb::destroy_debug_utils_messenger(m_instance, m_data->vkb_instance.debug_messenger);
    vkDestroyInstance(m_instance, nullptr);
}

void Core::destroy_queued_handles() {
    for (auto handle : m_dset_layouts) {
        vkDestroyDescriptorSetLayout(device(), handle, nullptr);
    }
}

void Core::immediate_submit(std::function<void(CommandBuffer& cmd)> function) {
    CommandBuffer cmd(this, true);
    Fence fence(this, false);

    cmd.begin();
    function(cmd);
    cmd.end();

    VkSubmitInfo info{
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,

        .commandBufferCount = 1,
        .pCommandBuffers    = &cmd.handle(),
    };

    CommandBuffer* cmds[] = {&cmd};
    fence.submit(&info, cmds);
    fence.wait();
}

} // namespace vke