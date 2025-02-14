#include "imgui_manager.hpp"
#include "render/window/window_sdl.hpp"

#include <backends/imgui_impl_sdl2.h>
#include <backends/imgui_impl_vulkan.h>

#include <vke/util.hpp>

#include <imgui.h>

namespace vke {

ImguiManager::ImguiManager(Window* window, Renderpass* renderpass, u32 subpass_index) {
    m_window = dynamic_cast<WindowSDL*>(window);

    // m_window->register_imgui_manager(this);

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

    auto ctx = VulkanContext::get_context();

    // this initializes imgui for Vulkan
    ImGui_ImplVulkan_InitInfo init_info = {
        .Instance       = ctx->get_instance(),
        .PhysicalDevice = ctx->get_physical_device(),
        .Device         = ctx->get_device(),
        .Queue          = ctx->get_graphics_queue(),
        .DescriptorPool = m_imgui_pool,
        .RenderPass = renderpass->handle(),
        .MinImageCount  = 3,
        .ImageCount     = 3,
        .MSAASamples    = VK_SAMPLE_COUNT_1_BIT,
        .Subpass        = subpass_index,
    };
    ImGui_ImplVulkan_Init(&init_info);

    ImGui_ImplVulkan_CreateFontsTexture();

    // ImGui_ImplVulkan_DestroyFontUploadObjects();
}

ImguiManager::~ImguiManager() {
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    vkDestroyDescriptorPool(device(), m_imgui_pool, nullptr);
    ImGui::DestroyContext();
}

void ImguiManager::new_frame() {
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplSDL2_NewFrame();

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