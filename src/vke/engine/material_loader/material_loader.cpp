#include "material_loader.hpp"

#include <exception>
#include <filesystem>

#include "../../core/image.hpp"
#include "../../core/pipeline_builder.hpp"
#include "../../core/renderpass.hpp"
#include "../../util.hpp"

#include "../../serialization/json_serialization.hpp"

namespace fs = std::filesystem;

namespace vke {

void MaterialLoader::load_shader(const ShaderDescription& description) {
    GPipelineBuilder builder(core());

    for (auto& path : description.shader_paths) {
        builder.add_shader_stage(path);
    }

    auto* render_target = m_material_manager->get_render_target(description.render_target);
    if (!render_target) throw std::runtime_error("render target couldn't be found");

    builder.set_depth_testing(render_target->depth_format.has_value());
    builder.set_renderpass(render_target->renderpass, render_target->subpass_index);
    if (description.vertex_input.has_value()) {
        builder.set_vertex_input(m_material_manager->get_vertex_input(*description.vertex_input));
    }

    m_material_manager->register_shader(std::make_unique<Shader>(Shader{
        .pipeline           = builder.build(),
        .material_set_index = description.material_set_index,
        .name               = description.name,
    }));
}

void MaterialLoader::load_material(CommandBuffer& cmd, const MaterialDescription& description) {
    m_material_manager->register_material(std::make_unique<Material>(Material{
        .shader     = m_material_manager->get_shader(description.shader_name),
        .textures   = map_vec(description.texture_paths, [&, this](const std::string& path) {
            if (path[0] == '#') return m_material_manager->get_texture(path);
            return load_image(cmd, path);
        }),
        .config_ubo = [&] {
            if (auto& path = description.buffer_path) {
                return m_material_manager->get_buffer(description.buffer_path.value());
            } else {
                return std::shared_ptr<vke::Buffer>(nullptr);
            }
        }(),
        .name = description.name,
    }));
}

std::shared_ptr<Image> MaterialLoader::load_image(CommandBuffer& cmd, const std::string& path) {
    auto it = m_image_cache.find(path);
    if (it != m_image_cache.end()) {
        return it->second;
    }

    std::shared_ptr<Image> image = Image::load_png(cmd, path.c_str());
    m_image_cache.emplace(path, image);

    return image;
}

void MaterialLoader::load_material_pack(CommandBuffer& cmd, const MaterialPack& pack) {
    for (auto& shader : pack.shaders) {
        load_shader(shader);
    }

    for (auto& material : pack.materials) {
        load_material(cmd, material);
    }
}

void MaterialLoader::load_material_file(CommandBuffer& cmd, const char* filename) {
    JsonDeserializer deserializer(read_file(filename));
    MaterialPack pack;
    deserializer.pull_root(pack);

    auto base_dir = fs::path(filename).parent_path();

    auto update_path = [&](std::string& path) {
        if (path[0] == '#') return;
        path = (base_dir / path).string();
    };

    auto update_paths = [&](std::vector<std::string>& paths) {
        for (auto& path : paths)
            update_path(path);
    };

    for (auto& shader : pack.shaders) {
        update_paths(shader.shader_paths);
    }

    for (auto& material : pack.materials) {
        update_paths(material.texture_paths);
    }

    load_material_pack(cmd, pack);
}

} // namespace vke
