#pragma once

#include <optional>

#include "../serialization/serializer.hpp"

#include "../common.hpp"
#include "../fwd.hpp"
#include "../material_manager.hpp"

namespace vke {

struct ShaderDescription {
    std::string name;
    std::string render_target;
    std::optional<std::string> vertex_input;
    std::vector<std::string> shader_paths;
    u32 material_set_index;

    AUTO_SERIALIZATON(ShaderDescription, name, render_target, vertex_input, shader_paths, material_set_index);
};

struct MaterialDescription {
    std::string name;
    std::string shader_name;//name of the shader used
    std::vector<std::string> texture_paths;

    AUTO_SERIALIZATON(MaterialDescription, name,shader_name,texture_paths);
};

struct MaterialPack{
    std::vector<ShaderDescription> shaders;
    std::vector<MaterialDescription> materials;

    AUTO_SERIALIZATON(MaterialPack, shaders,materials);
};


class MaterialLoader {
public:
    MaterialLoader(MaterialManager* manager) { m_material_manager = manager; }

    void load_shader(const ShaderDescription& description);
    void load_material(CommandBuffer& cmd, const MaterialDescription& description);

    void load_material_pack(CommandBuffer& cmd,const MaterialPack& pack);
    void load_material_file(CommandBuffer& cmd,const char* filename);


private:
    Core* core() { return m_material_manager->core(); }

    std::shared_ptr<Image> load_image(CommandBuffer& cmd, const std::string& path);

private:
    MaterialManager* m_material_manager;
    std::unordered_map<std::string, std::shared_ptr<Image>> m_image_cache;
};

} // namespace vke