#include "scene_data.hpp"

#include <vke/vke.hpp>
#include <vke/vke_builders.hpp>

namespace vke {

SceneData::SceneData(RenderServer* engine) {
    vke::DescriptorSetLayoutBuilder builder;
    
    builder.add_binding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL, 1);

    m_descriptor_set_layout = builder.build();
}

SceneData::~SceneData() {}
} // namespace vke