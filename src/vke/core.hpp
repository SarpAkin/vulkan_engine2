#pragma once

#include <memory>

#include "common.hpp"
#include "fwd.hpp"

#include "sampler_manager.hpp"

#include <functional>
#include <span>
#include <vector>
#include <vulkan/vulkan_core.h>

typedef struct VmaAllocator_T* VmaAllocator;
typedef struct VmaAllocation_T* VmaAllocation;

namespace vke {
struct CoreConfig;
struct ImageArgs;

constexpr usize FRAME_OVERLAP = 2;

class Core {

public: // getters
    inline VkInstance instance() const { return m_instance; }
    inline VkDevice device() const { return m_device; }
    inline VkQueue queue() const { return m_graphics_queue; }
    inline u32 queue_family() const { return m_graphics_queue_family; }
    inline VmaAllocator gpu_allocator() { return m_gpu_allocator; }
    inline VkPhysicalDevice physical_device() const { return m_chosen_gpu; }



public: // util
    inline usize gpu_allignment() const { return 256; }
    usize pad_buffer(usize bsize) const;

    SamplerManager* get_sampler_manager() const { return m_sampler_manager; }

    void immediate_submit(std::function<void(CommandBuffer& cmd)> function);

public: // resource creation
    // these are located individually at the resources cpp file. ex: buffer.cpp
    std::unique_ptr<Buffer> create_buffer(VkBufferUsageFlagBits usage, usize buffer_size, bool host_visible /* whether it is accessible by cpu*/);
    std::unique_ptr<Buffer> create_buffer_from_data(VkBufferUsageFlagBits usage, std::span<u8> bytes); // creates host visible data initalized with given bytes
    template <typename T, usize N = std::dynamic_extent>
    std::unique_ptr<Buffer> create_buffer_from_data(VkBufferUsageFlagBits usage, std::span<T, N> data) {
        return create_buffer_from_data(usage, std::span<u8>(reinterpret_cast<u8*>(data.data()), data.size_bytes()));
    }

    std::unique_ptr<Image> create_image(ImageArgs);
    std::unique_ptr<Fence> create_fence(bool signaled);
    std::unique_ptr<Semaphore> create_semaphore();
    std::unique_ptr<CommandBuffer> create_cmd(bool is_primary);

public: // constructors
    ~Core();
    Core(CoreConfig* config);

    // shoudn't be called by lib user
    inline void queue_destroy(VkDescriptorSetLayout layout) { m_dset_layouts.push_back(layout); }

private:
    void init_allocator();
    void init_managers();
    void destroy_queued_handles();

private: // private fields
    VkInstance m_instance;
    VkDevice m_device;
    VkPhysicalDevice m_chosen_gpu;

    VkQueue m_graphics_queue;    // queue we will submit to
    u32 m_graphics_queue_family; // family of that queue

    VmaAllocator m_gpu_allocator;

    struct CoreData;
    std::unique_ptr<CoreData> m_data;

    // handles to be destroyed before core destruction
    std::vector<VkDescriptorSetLayout> m_dset_layouts;

    // owned by core
    SamplerManager* m_sampler_manager = nullptr;
};

struct CoreConfig {
    const char* app_name = "Default App Name";
    u32 vk_version_major = 1;
    u32 vk_version_minor = 3;
    u32 vk_version_patch = 0;
    Window* window       = nullptr;
};

} // namespace vke