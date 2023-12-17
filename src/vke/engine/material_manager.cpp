#include "material_manager.hpp"

#include "../core/buffer_reflection.hpp"
#include "../core/commandbuffer.hpp"
#include "../core/descriptor_pool.hpp"
#include "../core/descriptor_set_builder.hpp"
#include "../core/image.hpp"
#include "../core/pipeline.hpp"
#include "../core/pipeline_reflection.hpp"
#include "../core/renderpass.hpp"
#include "../util.hpp"

#include <imgui.h>

namespace vke {

MaterialManager::~MaterialManager() {}

void MaterialManager::debug_gui() {
    static bool is_debug_open = true;
    ImGui::Begin("shader debug buffers", &is_debug_open);

    for (auto& [name, shader] : m_shaders) {
        if (!shader->debug_buffer_index.has_value()) continue;

        ImGui::BeginChild(name.c_str(), ImVec2(400, 0), true);

        auto bound_ubo = shader->debug_ubo_reflection->bind(shader->debug_ubo.get());
        bound_ubo.for_each_field([](const std::string& fname, FieldAccesor& field) {
            switch (field.get_type()) {
            case BufferReflection::Field::UINT:
                break;
            case BufferReflection::Field::INT:
                ImGui::InputInt(fname.c_str(), &field.get_as<int>());
                break;
            case BufferReflection::Field::FLOAT:
                // ImGui::DragFloat(const char *label, float *v)
                ImGui::DragFloat(fname.c_str(), &field.get_as<float>(), 0.0, 1.0);
                break;
            case BufferReflection::Field::BOOL:
                ImGui::Checkbox(fname.c_str(), &field.get_as<bool>());
                break;
            case BufferReflection::Field::VEC2:
                break;
            case BufferReflection::Field::VEC3:
                ImGui::ColorPicker4(fname.c_str(), &field.get_as<float>());
                break;
            case BufferReflection::Field::VEC4:
                break;
            case BufferReflection::Field::IVEC2:
                break;
            case BufferReflection::Field::IVEC3:
                break;
            case BufferReflection::Field::IVEC4:
                break;
            default:
                break;
            }
        });

        ImGui::EndChild();
    }

    ImGui::End();
}

void Material::bind(CommandBuffer& cmd) {
    cmd.bind_pipeline(shader->pipeline.get());
    if (dset)
        cmd.bind_descriptor_set(shader->material_set_index.value(), dset);
}

void CommandBuffer::bind_material(Material* material) {
    material->bind(*this);
}

void Material::build_dset(DescriptorPool* pool) {
    if (!shader->material_set_index.has_value()) return;
    auto descrriptor_layout = shader->pipeline->get_descriptor_layout(shader->material_set_index.value());
    if (descrriptor_layout == nullptr) {
        dset = nullptr;
        return;
    }

    DescriptorSetBuilder builder;
    if (!textures.empty()) {
        builder.add_image_samplers(MAP_VEC_ALLOCA(textures, [](std::shared_ptr<vke::Image> ptr) { return ptr.get(); }),
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, sampler, VK_SHADER_STAGE_FRAGMENT_BIT);
    }

    if (shader->debug_buffer_index) {
        builder.add_ubo(shader->debug_ubo.get(), shader->debug_ubo_reflection->get_stages());
    }

    dset = builder.build(pool, descrriptor_layout);
}

MaterialManager::MaterialManager(RenderEngine* engine) : SystemBase(engine) {
    m_pool = std::make_unique<DescriptorPool>(core(), 100);
}

void MaterialManager::register_shader(std::unique_ptr<Shader> shader) {
    if (shader->material_set_index && shader->debug_buffer_index) {
        shader->debug_ubo_reflection = shader->pipeline->get_reflection()->reflect_buffer(shader->material_set_index.value(), shader->debug_buffer_index.value());
        shader->debug_ubo            = core()->create_buffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, shader->debug_ubo_reflection->get_buffer_size(), true);
    }

    m_shaders.emplace(shader->name, std::move(shader));
}
void MaterialManager::register_material(std::unique_ptr<Material> material) {
    material->build_dset(m_pool.get());
    m_materials.emplace(material->material_name(), std::move(material));
}
void MaterialManager::register_render_target(std::string name, Renderpass* renderpass, u32 subpass_index) {
    m_render_targets.emplace(name, renderpass->get_subpass(subpass_index));
}
void MaterialManager::register_vertex_input(std::string name, VertexInputDescriptionBuilder vertex_input) {
    m_vertex_inputs.emplace(name, std::make_unique<VertexInputDescriptionBuilder>(std::move(vertex_input)));
}

} // namespace vke