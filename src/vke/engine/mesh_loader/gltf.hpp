#pragma once

#include <glm/vec2.hpp>

#include "../../serialization/glm.hpp"
#include "../../serialization/serializer.hpp"

namespace vke {

namespace gltf {
    
#define SERIALIZE_FIELD(field) ser->insert(STRINGIFY(field), field);
#define DESERIALIZE_FIELD(field) deser->get_field(STRINGIFY(field), field);

struct GLTFScene {
    std::vector<i32> nodes;

    AUTO_SERIALIZATON(GLTFScene, nodes);
};

struct GLTFNode {
    i32 mesh;
    std::vector<i32> childIndices;

    AUTO_SERIALIZATON(GLTFNode, mesh, childIndices);
};

struct GLTFAttributes {
    std::optional<i32> POSITION, NORMAL;
    // ... other attribute properties

    AUTO_SERIALIZATON(GLTFAttributes, POSITION, NORMAL);
};

struct GLTFPrimitive {
    GLTFAttributes attributes;
    i32 indices;

    AUTO_SERIALIZATON(GLTFPrimitive, attributes, indices);
};

struct GLTFBuffer {
    std::string uri;
    i32 byteLength;

    AUTO_SERIALIZATON(GLTFBuffer, uri, byteLength);
};

struct GLTFMesh {
    std::vector<GLTFPrimitive> primitives;

    AUTO_SERIALIZATON(GLTFMesh, primitives);
};

struct GLTFBufferView {
    i32 buffer;
    i32 byteOffset;
    i32 byteLength;
    i32 target;

    AUTO_SERIALIZATON(GLTFBufferView, buffer, byteOffset, byteLength, target);
};

struct GLTFAccessor {
    i32 bufferView;
    i32 byteOffset;
    i32 componentType;
    i32 count;
    std::string type;
    glm::vec3 max;
    glm::vec3 min;

    AUTO_SERIALIZATON(GLTFAccessor, bufferView, byteOffset, componentType, count, type, max, min);
};

struct GLTFAssetVersion {
    std::string version;

    AUTO_SERIALIZATON(GLTFAssetVersion, version);
};

struct GLTFAsset {
    i32 scene;
    std::vector<GLTFScene> scenes;
    std::vector<GLTFNode> nodes;
    std::vector<GLTFMesh> meshes;
    std::vector<GLTFBuffer> buffers;
    std::vector<GLTFBufferView> bufferViews;
    std::vector<GLTFAccessor> accessors;
    GLTFAssetVersion asset;

    AUTO_SERIALIZATON(GLTFAsset, scene, scenes, nodes, meshes, buffers, bufferViews, accessors, asset);
};

} // namespace gltf

} // namespace vke