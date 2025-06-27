#include "gltf_loader.hpp"

#include <filesystem>
#include <flecs/addons/flecs_cpp.h>

#include "flecs/addons/cpp/world.hpp"
#include "glm/ext/matrix_float4x4.hpp"
#include "render/object_renderer/object_renderer.hpp"
#include "render/object_renderer/resource_manager.hpp"
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

    fs::path path = file_path;
    fs::path ext  = path.extension();

    bool ret;
    if (ext == ".gltf") {
        ret = loader.LoadASCIIFromFile(&model, &err, &warn, file_path);
    } else if (ext == ".glb") {
        ret = loader.LoadBinaryFromFile(&model, &err, &warn, file_path);
    } else {
        LOG_ERROR("unknown extension: %s", ext.c_str());
        return false;
    }

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

std::optional<flecs::entity> load_gltf_file(vke::CommandBuffer& cmd, flecs::world* world, ObjectRenderer* renderer, const std::string& file_path) {
    tg::Model model;
    if (!load_gltf_into_model(model, file_path)) return std::nullopt;

    auto* resource_manager = renderer->get_resource_manager();

    StencilBuffer stencil = StencilBuffer(1 << 21);

    std::string registered_name_prefix = file_path + "$";

    auto make_name = [&](const std::string& base_name) {
        return base_name.empty() ? std::string() : registered_name_prefix + base_name;
    };

    auto get_buffer_view = [&]<typename T>(Type<T>, int buffer_view, size_t byte_offset = 0) -> std::span<T> {
        auto& view   = model.bufferViews.at(buffer_view);
        auto& buffer = model.buffers.at(view.buffer);

        size_t byte_end = view.byteOffset + view.byteLength;
        if (buffer.data.size() < byte_end) {
            THROW_ERROR("while loading gltf file %s:buffer view %d overflows buffer %d by %ld bytes\n", file_path.c_str(), buffer_view, view.buffer, byte_end - buffer.data.size());
        }

        if (view.byteLength < byte_offset) {
            THROW_ERROR("while loading gltf file %s: accessor overflow by %ld", file_path.c_str(), byte_offset - view.byteLength);
        }

        // return std::span<T>(reinterpret_cast<T*>(buffer.data.data() + view.byteOffset + byte_offset), view.byteLength / sizeof(T));
        return vke::span_cast<T>(std::span(buffer.data).subspan(view.byteOffset + byte_offset, view.byteLength - byte_offset));
    };

    auto get_buffer_view_from_accessor = [&]<typename T>(Type<T> t, int accessor_index) {
        auto& accessor = model.accessors.at(accessor_index);

        return get_buffer_view(t, accessor.bufferView, accessor.byteOffset).subspan(0, accessor.count);
    };

    // model.textures[0].source
    // model.images[0].bufferView

    auto images = vke::map_vec(model.images, [&](const tg::Image& image) {
        if (!(image.bits == 8 && image.pixel_type == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE && image.component == 4)) {
            // handle unsupported formats
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

        return resource_manager->create_image(std::move(vke_image));
    });

    auto get_image = [&](int texture_index) {
        auto& texture = model.textures.at(texture_index);

        return images.at(texture.source);
    };

    auto default_id   = resource_manager->create_material(ObjectRenderer::pbr_pipeline_name, {});
    auto material_ids = vke::map_vec(model.materials, [&](const tg::Material& material) {
        int pbr_texture_index = material.pbrMetallicRoughness.baseColorTexture.index;
        if (pbr_texture_index == -1) {
            return default_id;
        }

        return resource_manager->create_material(ObjectRenderer::pbr_pipeline_name, {get_image(pbr_texture_index)});
    });

    auto set_indicies = [&](MeshBuilder& builder, int ib_accessor_index) {
        if (ib_accessor_index == -1) return;

        auto& index_accessor = model.accessors.at(ib_accessor_index);

        if (index_accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT) {
            builder.set_indicies(get_buffer_view_from_accessor(Type<uint16_t>(), ib_accessor_index));
            return;
        }

        if (index_accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT) {
            builder.set_indicies(get_buffer_view_from_accessor(Type<uint32_t>(), ib_accessor_index));
            return;
        }

        TODO();
    };

    auto create_mesh_from_primitive = [&](const tg::Primitive& primitive) {
        //"TEXCOORD_0" "NORMAL" "POSITION"
        // auto& position_accessor = model.accessors[primitive.attributes.at("POSITION")];

        auto position_view       = get_buffer_view_from_accessor(Type<glm::vec3>(), primitive.attributes.at("POSITION"));
        auto texture_coords_view = get_buffer_view_from_accessor(Type<glm::vec2>(), primitive.attributes.at("TEXCOORD_0"));
        auto normals_view        = get_buffer_view_from_accessor(Type<glm::vec3>(), primitive.attributes.at("NORMAL"));

        auto& position_accessor = model.accessors[primitive.attributes.at("POSITION")];

        MeshBuilder builder;
        builder.set_positions(position_view);
        builder.set_texture_coords(texture_coords_view);
        builder.set_normals(normals_view);

        builder.set_boundary(AABB{
            .start = {position_accessor.minValues[0], position_accessor.minValues[1], position_accessor.minValues[2]},
            .end   = {position_accessor.maxValues[0], position_accessor.maxValues[1], position_accessor.maxValues[2]},
        });

        set_indicies(builder, primitive.indices);

        return resource_manager->create_mesh(builder.build(&cmd, &stencil));
    };

    auto model_ids = vke::map_vec(model.meshes, [&](const tg::Mesh& mesh) {
        auto parts = vke::map_vec(mesh.primitives, [&](const tg::Primitive& primitive) {
            return std::pair(create_mesh_from_primitive(primitive), material_ids.at(primitive.material));
        });

        return resource_manager->create_model(parts, make_name(mesh.name));
    });

    auto create_transformation_from_node = [&](tg::Node& node) {
        if (node.matrix.size() == 16) {
            glm::mat4 mat(1);
            for (int i = 0; i < 16; i++) {
                mat[i / 4][i % 4] = static_cast<float>(node.matrix[i]);
            }
            return RelativeTransform::decompose_from_matrix(mat);
        }

        RelativeTransform transform;

        if (node.translation.size() == 3) {
            transform.position[0] = static_cast<float>(node.translation[0]);
            transform.position[1] = static_cast<float>(node.translation[1]);
            transform.position[2] = static_cast<float>(node.translation[2]);
        } else {
            transform.position = {0, 0, 0};
        }

        transform.scale    = {1, 1, 1};
        transform.rotation = glm::quat(1, 0, 0, 0);

        assert(node.rotation.empty());
        assert(node.scale.empty());

        return transform;
    };

    auto convert_node = vke::make_y_combinator([&](auto& self, int node_index, std::optional<flecs::entity> parent = std::nullopt) -> flecs::entity {
        auto& node     = model.nodes.at(node_index);
        auto transform = create_transformation_from_node(node);

        auto e = world->prefab();

        if (parent.has_value()) {
            e.child_of(parent.value());
        }

        e.set<RelativeTransform>(transform);

        if (node.mesh != -1) {
            auto model_id = model_ids[node.mesh];

            e.set<Renderable>(Renderable{model_ids.at(node.mesh)});
        }

        for (auto child_index : node.children) {
            self(child_index, e);
        }

        return e;
    });

    auto res  = convert_node(0);

    stencil.flush_copies(cmd);

    return res;
}
} // namespace vke