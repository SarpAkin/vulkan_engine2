#include "imgui_manager.hpp"

#include "../commandbuffer.hpp"
#include "../core.hpp"
#include "../renderpass.hpp"
#include "../vkutil.hpp"
#include "../window_sdl.hpp"

#include <backends/imgui_impl_sdl2.h>
#include <backends/imgui_impl_vulkan.h>

#include <imgui.h>

namespace vke {

ImguiManager::ImguiManager(Core* core_, Window* window, Renderpass* renderpass, u32 subpass_index) : Resource(core_) {
    m_window = dynamic_cast<Window_SDL*>(window);

    m_window->register_imgui_manager(this);

    u32 descriptor_count = 250;

    VkDescriptorPoolSize pool_sizes[] = {
        {VK_DESCRIPTOR_TYPE_SAMPLER, descriptor_count},
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, descriptor_count},
        {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, descriptor_count},
        {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, descriptor_count},
        {VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, descriptor_count},
        {VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, descriptor_count},
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, descriptor_count},
        {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, descriptor_count},
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, descriptor_count},
        {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, descriptor_count},
        {VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, descriptor_count} //
    };

    VkDescriptorPoolCreateInfo pool_info = {
        .sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .flags         = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
        .maxSets       = descriptor_count,
        .poolSizeCount = std::size(pool_sizes),
        .pPoolSizes    = pool_sizes,
    };

    VK_CHECK(vkCreateDescriptorPool(device(), &pool_info, nullptr, &m_imgui_pool));

    ImGui::CreateContext();

    ImGui_ImplSDL2_InitForVulkan(m_window->handle());

    // this initializes imgui for Vulkan
    ImGui_ImplVulkan_InitInfo init_info = {
        .Instance       = core()->instance(),
        .PhysicalDevice = core()->physical_device(),
        .Device         = device(),
        .Queue          = core()->queue(),
        .DescriptorPool = m_imgui_pool,
        .Subpass        = subpass_index,
        .MinImageCount  = 3,
        .ImageCount     = 3,
        .MSAASamples    = VK_SAMPLE_COUNT_1_BIT,
    };
    ImGui_ImplVulkan_Init(&init_info, renderpass->handle());

    core()->immediate_submit([&](CommandBuffer& cmd) {
        ImGui_ImplVulkan_CreateFontsTexture(cmd.handle());
    });

    ImGui_ImplVulkan_DestroyFontUploadObjects();
}

ImguiManager::~ImguiManager() {
    ImGui_ImplVulkan_Shutdown();
    vkDestroyDescriptorPool(device(), m_imgui_pool, nullptr);
    ImGui::DestroyContext();
}

void ImguiManager::new_frame() {
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplSDL2_NewFrame(m_window->handle());
    
    ImGui::NewFrame();
}

void ImguiManager::flush_frame(CommandBuffer& cmd) {
    ImGui::Render();
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd.handle());
}

void ImguiManager::process_sdl_event(SDL_Event& event) {
    ImGui_ImplSDL2_ProcessEvent(&event);
}

} // namespace vke