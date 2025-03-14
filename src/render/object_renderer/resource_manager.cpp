#include "resource_manager.hpp"

#include <vke/pipeline_loader.hpp>
#include <vke/vke_builders.hpp>

#include "render/render_server.hpp"
#include "render_state.hpp"

namespace vke {

ResourceManager::ResourceManager(RenderServer* render_server) : m_render_server(render_server) {
    vke::DescriptorSetLayoutBuilder layout_builder;
    layout_builder.add_image_sampler(VK_SHADER_STAGE_FRAGMENT_BIT, 4);
    // layout_builder.add_ubo(VK_SHADER_STAGE_ALL, 1);
    m_material_set_layout = layout_builder.build();

    m_descriptor_pool = std::make_unique<vke::DescriptorPool>();

    create_null_texture(16);

    VkSamplerCreateInfo sampler_info{
        .sType        = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
        .magFilter    = VK_FILTER_NEAREST,
        .minFilter    = VK_FILTER_NEAREST,
        .addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT,
        .addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT,
        .addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT,
    };

    VK_CHECK(vkCreateSampler(device(), &sampler_info, nullptr, &m_nearest_sampler));
}

ResourceManager::~ResourceManager() {
    vkDestroyDescriptorSetLayout(device(), m_material_set_layout, nullptr);
    vkDestroySampler(device(), m_nearest_sampler, nullptr);
}

void ResourceManager::create_multi_target_pipeline(const std::string& name, std::span<const std::string> pipeline_names) {
    MultiPipeline multi_pipeline{};

    for (const auto& name : pipeline_names) {
        auto pipeline = load_pipeline_cached(name);
        
        auto subpass_name = std::string(pipeline->subpass_name());
        multi_pipeline.pipelines[subpass_name] = std::move(pipeline);
    }

    m_multi_pipelines[name] = std::move(multi_pipeline);
}

MaterialID ResourceManager::create_material(const std::string& pipeline_name, std::vector<ImageID> images, const std::string& material_name) {
    images.resize(4, m_null_texture_id);

    Material m{
        .multi_pipeline = &m_multi_pipelines.at(pipeline_name),
        .material_set   = VK_NULL_HANDLE,
        .images         = images,
        .name           = material_name,
    };

    auto views = vke::map_vec(images, [&](auto& id) {
        return std::pair(get_image(id), m_nearest_sampler);
    });

    vke::DescriptorSetBuilder builder;
    builder.add_image_samplers(views, VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL, VK_SHADER_STAGE_FRAGMENT_BIT);
    m.material_set = builder.build(m_descriptor_pool.get(), m_material_set_layout);

    auto id = MaterialID(m_material_id_manager.new_id());

    if (!material_name.empty()) {
        assert(!m_material_names2material_ids.contains(material_name) && "material name is already present");
        m_material_names2material_ids[material_name] = id;
    }

    m_materials[id] = std::move(m);
    m_updates.material_updates.push_back(id);
    return id;
}

MeshID ResourceManager::create_mesh(Mesh mesh, const std::string& name) {
    auto id = MeshID(m_mesh_id_manager.new_id());

    m_meshes[id] = std::move(mesh);

    if (!name.empty()) {
        assert(!m_mesh_names2mesh_ids.contains(name) && "mesh name is already present");
        m_mesh_names2mesh_ids[name] = id;
    }

    m_updates.mesh_updates.push_back(id);
    return id;
}

void ResourceManager::calculate_boundary(RenderModel& model) {
    using Part = RenderModel::Part;

    auto boundary = vke::fold(model.parts, AABB{}, [&](const AABB& a, const Part& b) {
        return a.combined(get_mesh(b.mesh_id)->boundary);
    });

    model.boundary = boundary;
}

RenderModelID ResourceManager::create_model(MeshID mesh, MaterialID material, const std::string& name) {
    auto id = RenderModelID(m_render_model_id_manager.new_id());

    RenderModel model = {
        .parts = {
            {mesh, material},
        },
    };
    calculate_boundary(model);
    m_render_models[id] = std::move(model);

    if (!name.empty()) {
        bind_name2model(id, name);
    }

    m_updates.model_updates.push_back(id);
    return id;
}

RenderModelID ResourceManager::create_model(const std::vector<std::pair<MeshID, MaterialID>>& parts, const std::string& name) {
    auto id = RenderModelID(m_render_model_id_manager.new_id());

    RenderModel model = {
        .parts = map_vec(parts, [](auto& part) {
        auto& [mesh, mat] = part;
        return RenderModel::Part{mesh, mat};
    }),
    };
    calculate_boundary(model);
    m_render_models[id] = std::move(model);

    if (!name.empty()) {
        bind_name2model(id, name);
    }

    m_updates.model_updates.push_back(id);
    return id;
}

void ResourceManager::bind_name2model(RenderModelID id, const std::string& name) {
    assert(!m_render_model_names2model_ids.contains(name) && "model name is already present");

    m_render_model_names2model_ids[name] = id;
    m_render_models[id].name             = name;
}

ImageID ResourceManager::create_image(std::unique_ptr<IImageView> image_view, const std::string& name) {
    auto id = ImageID(m_image_id_manager.new_id());

    m_images[id] = std::move(image_view);

    if (!name.empty()) {
        m_image_names2image_ids[name] = id;
    }

    m_updates.image_updates.push_back(id);
    return id;
}

void ResourceManager::create_null_texture(int size) {
    auto image = std::make_unique<vke::Image>(ImageArgs{
        .format       = VK_FORMAT_R8G8B8A8_SRGB,
        .usage_flags  = VK_IMAGE_USAGE_SAMPLED_BIT,
        .width        = static_cast<u32>(size),
        .height       = static_cast<u32>(size),
        .layers       = 1,
        .host_visible = true,
    });

    auto pixels   = image->mapped_data_ptr<u32>();
    int half_size = size / 2;

    u32 BLACK  = 0xFF'00'00'00;
    u32 PURPLE = 0xFF'BB'00'BB;

    for (int y = 0; y < size; y++) {
        for (int x = 0; x < size; x++) {
            pixels[x + y * size] = (y > half_size) ^ (x > half_size) ? BLACK : PURPLE;
        }
    }

    m_null_texture    = image.get();
    m_null_texture_id = create_image(std::move(image), "null");

    VulkanContext::get_context()->immediate_submit([&](CommandBuffer& cmd) {
        VkImageMemoryBarrier barriers[] = {
            VkImageMemoryBarrier{
                .sType            = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
                .srcAccessMask    = VK_ACCESS_HOST_WRITE_BIT,
                .dstAccessMask    = VK_ACCESS_SHADER_READ_BIT,
                .oldLayout        = VK_IMAGE_LAYOUT_UNDEFINED,
                .newLayout        = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                .image            = m_null_texture->vke_image()->handle(),
                .subresourceRange = {
                    .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                    .levelCount = 1,
                    .layerCount = 1,
                }},
        };

        cmd.pipeline_barrier(PipelineBarrierArgs{
            .src_stage_mask        = VK_PIPELINE_STAGE_HOST_BIT,
            .dst_stage_mask        = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            .image_memory_barriers = barriers,
        });
    });
}

IImageView* ResourceManager::get_image(ImageID id) {
    auto it = m_images.find(id);
    if (it != m_images.end()) {
        return it->second.get();
    } else {
        return m_null_texture;
    }
}

RCResource<vke::IPipeline> ResourceManager::load_pipeline_cached(const std::string& name) {
    auto val = vke::at(m_cached_pipelines, name);
    if (val.has_value()) return val.value();

    RCResource<IPipeline> pipeline = m_render_server->get_pipeline_loader()->load(name.c_str());
    m_cached_pipelines[name]       = pipeline;

    return pipeline;
}

bool ResourceManager::bind_mesh(RenderState& state, MeshID id) {
    if (state.bound_mesh_id == id) return true;

    if (auto it = m_meshes.find(id); it != m_meshes.end()) {
        state.mesh = &it->second;
    } else {
        LOG_ERROR("failed to bind mesh with id %d", id.id);
        return false;
    }

    state.cmd.bind_index_buffer(state.mesh->index_buffer.get(), state.mesh->index_type);
    auto vbs = map_vec2small_vec<6>(state.mesh->vertex_buffers, [&](const auto& b) -> const IBufferSpan* { return b.get(); });
    state.cmd.bind_vertex_buffer(std::span(vbs));

    return true;
}

bool ResourceManager::bind_material(RenderState& state, MaterialID id) {
    if (state.bound_material_id == id) return true;

    if (auto it = m_materials.find(id); it != m_materials.end()) {
        state.material = &it->second;
    } else {
        LOG_ERROR("failed to bind material with id %d", id.id);
        return false;
    }

    auto pipeline = state.material->multi_pipeline->pipelines.at(state.render_target->renderpass_name).get();

    if (state.bound_pipeline != pipeline) {
        state.bound_pipeline = pipeline;
        state.cmd.bind_pipeline(state.bound_pipeline);
    }
    state.cmd.bind_descriptor_set(ObjectRenderer::MATERIAL_SET, state.material->material_set);

    return true;
}

} // namespace vke