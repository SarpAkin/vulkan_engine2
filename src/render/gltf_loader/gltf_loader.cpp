#include "gltf_loader.hpp"

#include <entt/entt.hpp>

#include "glm/ext/matrix_float4x4.hpp"
#include "tiny_gltf.h"

#include "scene/components/transform.hpp"

namespace vke {

namespace tg = tinygltf;

template <class T>
struct Type {};

static bool load_gltf_into_model(tg::Model& model, const std::string& file_path) {
    tg::TinyGLTF loader;
    std::string err;
    std::string warn;

    bool ret = loader.LoadASCIIFromFile(&model, &err, &warn, file_path);
    // bool ret = loader.LoadBinaryFromFile(&model, &err, &warn, argv[1]); // for binary glTF(.glb)

    if (!warn.empty()) {
        LOG_WARNING("Warn: %s\n", warn.c_str());
        return false;
    }

    if (!err.empty()) {
        LOG_ERROR("Err: %s\n", err.c_str());
        return false;
    }

    if (!ret) {
        LOG_ERROR("Failed to parse glTF\n");
        return false;
    }

    return true;
}

void load_gltf_file(CommandBuffer& cmd, entt::registry* registry, ObjectRenderer* renderer, const std::string& file_path) {
    tg::Model model;
    if (!load_gltf_into_model(model, file_path)) return;

    std::string registered_name_prefix = file_path + "$";

    auto make_name = [&](const std::string& base_name) {
        return base_name.empty() ? std::string() : registered_name_prefix + base_name;
    };

    auto get_buffer_view = [&]<typename T>(Type<T>, int buffer_view, size_t byte_offset = 0) -> std::span<T> {
        auto& view   = model.bufferViews[buffer_view];
        auto& buffer = model.buffers[view.buffer];

        // return std::span<T>(reinterpret_cast<T*>(buffer.data.data() + view.byteOffset + byte_offset), view.byteLength / sizeof(T));
        return vke::span_cast<T>(std::span(buffer.data).subspan(view.byteOffset + byte_offset, view.byteLength));
    };

    auto get_buffer_view_from_accesor = [&]<typename T>(Type<T> t, int accesor_index) {
        auto& accesor = model.accessors[accesor_index];

        return get_buffer_view(t, accesor.bufferView, accesor.byteOffset).subspan(0, accesor.count);
    };

    // model.textures[0].source
    // model.images[0].bufferView

    auto images = vke::map_vec(model.images, [&](const tg::Image& image) {
        if (!(image.bits == 8 && image.pixel_type == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE && image.component == 4)) {
            // handle unspported formats
            TODO();
        }

        auto vke_image = Image::image_from_bytes(cmd, std::span(image.image),
            vke::ImageArgs{
                .format      = VK_FORMAT_R8G8B8A8_SRGB,
                .usage_flags = VK_IMAGE_USAGE_SAMPLED_BIT,
                .width       = static_cast<u32>(image.width),
                .height      = static_cast<u32>(image.height),
                .layers      = 1,
            });

        return renderer->create_image(std::move(vke_image));
    });

    auto get_image = [&](int texture_index){
        auto& texture = model.textures[texture_index];
        
        return images[texture.source];
    };

    auto default_id   = renderer->try_get_material_id("vke::default_material").value();
    auto material_ids = vke::map_vec(model.materials, [&](const tg::Material& material) {
        int pbr_texture_index = material.pbrMetallicRoughness.baseColorTexture.index;
        if (pbr_texture_index == -1) return default_id;

        return renderer->create_material("vke::default", {get_image(pbr_texture_index)});
    });

    auto set_indicies = [&](MeshBuilder& builder, int ib_accesor_index) {
        if (ib_accesor_index == -1) return;

        auto& index_accesor = model.accessors[ib_accesor_index];

        if (index_accesor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT) {
            builder.set_indicies(get_buffer_view_from_accesor(Type<uint16_t>(), ib_accesor_index));
            return;
        }

        if (index_accesor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT) {
            builder.set_indicies(get_buffer_view_from_accesor(Type<uint32_t>(), ib_accesor_index));
            return;
        }

        TODO();
    };

    auto create_mesh_from_primative = [&](const tg::Primitive& primative) {
        //"TEXCOORD_0" "NORMAL" "POSITION"
        // auto& position_accesor = model.accessors[primative.attributes.at("POSITION")];

        auto position_view = get_buffer_view_from_accesor(Type<glm::vec3>(), primative.attributes.at("POSITION"));
        auto texture_coords_view = get_buffer_view_from_accesor(Type<glm::vec2>(), primative.attributes.at("TEXCOORD_0"));


        MeshBuilder builder;
        builder.set_positions(position_view);
        builder.set_texture_coords(texture_coords_view);

        set_indicies(builder, primative.indices);

        return renderer->create_mesh(builder.build());
    };

    auto model_ids = vke::map_vec(model.meshes, [&](const tg::Mesh& mesh) {
        auto parts = vke::map_vec(mesh.primitives, [&](const tg::Primitive& primative) {
            return std::pair(create_mesh_from_primative(primative), material_ids[primative.material]);
        });

        return renderer->create_model(parts, make_name(mesh.name));
    });

    auto create_transformation_from_node = [&](tg::Node& node) {
        glm::mat4 mat(1);
        if (node.matrix.size() == 16) {
            for (int i = 0; i < 16; i++) {
                mat[i / 4][i % 4] = static_cast<float>(node.matrix[i]);
            }
            return mat;
        }

        if (node.translation.size() == 3) {
            mat[3][0] = static_cast<float>(node.translation[0]);
            mat[3][1] = static_cast<float>(node.translation[1]);
            mat[3][2] = static_cast<float>(node.translation[2]);
        }

        return mat;
    };

    auto convert_node = vke::make_y_combinator([&](auto& self, int node_index, const glm::mat4& t) -> void {
        auto& node         = model.nodes[node_index];
        auto transform_mat = t * create_transformation_from_node(node);

        auto transform = Transform::decompose_from_matrix(transform_mat);

        auto entity = registry->create();
        registry->emplace<Transform>(entity, transform);
        registry->emplace<Renderable>(entity, Renderable{model_ids[node.mesh]});

        for (auto child_index : node.children) {
            self(child_index, transform_mat);
        }
    });

    convert_node(0, glm::mat4(1));
}

} // namespace vke