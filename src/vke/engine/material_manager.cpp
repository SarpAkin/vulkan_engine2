#include "material_manager.hpp"

#include "../core/commandbuffer.hpp"
#include "../core/descriptor_pool.hpp"
#include "../core/descriptor_set_builder.hpp"
#include "../core/image.hpp"
#include "../core/pipeline.hpp"
#include "../core/renderpass.hpp"
#include "../util.hpp"

namespace vke {

MaterialManager::~MaterialManager() {}

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
    builder.add_image_samplers(MAP_VEC_ALLOCA(textures, [](std::shared_ptr<vke::Image> ptr) { return ptr.get(); }),
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, pool->core()->get_sampler_manager()->nearest_sampler(), VK_SHADER_STAGE_FRAGMENT_BIT);
    dset = builder.build(pool, descrriptor_layout);
}

MaterialManager::MaterialManager(RenderEngine* engine) : SystemBase(engine) {
    m_pool = std::make_unique<DescriptorPool>(core(), 100);
}

void MaterialManager::register_shader(std::unique_ptr<Shader> shader) {
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