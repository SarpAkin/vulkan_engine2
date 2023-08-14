#pragma once

#include "../common.hpp"
#include "../fwd.hpp"
#include "vk_system.hpp"

#include "../core/buffer.hpp"
#include "../core/renderpass.hpp"
#include "../core/vertex_input_builder.hpp"

namespace vke {

struct SubpassDetails;
class MaterialLoader;

class Shader {
public:
    std::unique_ptr<Pipeline> pipeline;
    u32 material_set_index;
    std::string name;

private:
};

class Material {
public:
    const std::string& material_name() const { return name; }

    void bind(CommandBuffer& cmd);
    void build_dset(DescriptorPool* pool);

public:
    Shader* shader       = nullptr;
    VkDescriptorSet dset = nullptr;
    std::vector<std::shared_ptr<Image>> textures;
    std::shared_ptr<Buffer> config_ubo = nullptr;
    std::string name;
};

class MaterialManager : public SystemBase {
public:
    MaterialManager(RenderEngine* engine);
    ~MaterialManager();

    Material* get_material(const std::string& name) const {
        auto it = m_materials.find(name);
        return it != m_materials.end() ? it->second.get() : nullptr;
    }

    Shader* get_shader(const std::string& name) const {
        auto it = m_shaders.find(name);
        return it != m_shaders.end() ? it->second.get() : nullptr;
    }

    const SubpassDetails* get_render_target(const std::string& name) const {
        auto it = m_render_targets.find(name);
        return it != m_render_targets.end() ? it->second : nullptr;
    }

    const VertexInputDescriptionBuilder* get_vertex_input(const std::string& name) const {
        auto it = m_vertex_inputs.find(name);
        return it != m_vertex_inputs.end() ? it->second.get() : nullptr;
    }

    const std::shared_ptr<Buffer> get_buffer(const std::string& name){
        assert(name[0] == '#' && "registered image  should start must have the '#' prefix!");
        return m_config_ubos[name];
    }
    const std::shared_ptr<Image> get_texture(const std::string& name){
        assert(name[0] == '#' && "registered buffer should start must have the '#' prefix!");
        return m_textures[name];
    }
    

    void register_shader(std::unique_ptr<Shader> shader);
    void register_material(std::unique_ptr<Material> material);
    void register_render_target(std::string name, Renderpass* renderpass, u32 subpass_index);
    void register_vertex_input(std::string name, VertexInputDescriptionBuilder vertex_input);
    void register_texture(std::string name, std::shared_ptr<vke::Image> texture);
    void register_buffer(std::string name, std::shared_ptr<vke::Buffer> buffer);

private:
    std::unique_ptr<DescriptorPool> m_pool;

    std::unordered_map<std::string, std::shared_ptr<vke::Image>> m_textures;
    std::unordered_map<std::string, std::shared_ptr<vke::Buffer>> m_config_ubos;

    std::unordered_map<std::string, std::unique_ptr<Shader>> m_shaders;
    std::unordered_map<std::string, std::unique_ptr<Material>> m_materials;
    std::unordered_map<std::string, const SubpassDetails*> m_render_targets;
    std::unordered_map<std::string, std::unique_ptr<VertexInputDescriptionBuilder>> m_vertex_inputs;
};

} // namespace vke