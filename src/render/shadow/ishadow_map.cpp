#include "ishadow_map.hpp"

namespace vke {

void IShadowMap::execute_late_rasters(vke::CommandBuffer& primary_cmd, std::vector<LateRasterData>& raster_buffers) {
    for (auto& rb : raster_buffers) {
        rb.shadow_renderpass->set_active_frame_buffer_instance(rb.layer_index);

        rb.shadow_renderpass->set_external(true);
        rb.shadow_renderpass->begin(primary_cmd);

        primary_cmd.execute_secondaries(rb.render_buffer.get());
        rb.shadow_renderpass->end(primary_cmd);

        primary_cmd.add_execution_dependency(rb.render_buffer->get_reference());
    }
}
} // namespace vke