#pragma once

#include <vector>

#include "../core/core.hpp"
#include "../fwd.hpp"

#include <cassert>

namespace vke {

class SystemBase {
public:
    inline Core* core() const { return m_core; }
    inline RenderEngine* engine() const { return m_engine; }
    VkDevice device() const;

    SystemBase(RenderEngine* engine);
    virtual ~SystemBase(){};

    SystemBase(const SystemBase&)            = delete;
    SystemBase(SystemBase&&)                 = delete;
    SystemBase& operator=(const SystemBase&) = delete;
    SystemBase& operator=(SystemBase&&)      = delete;

    u32 get_frame_index() const;
    u32 get_frame_overlap() const;

    DescriptorPool* get_framely_descriptor_pool();

private:
    Core* m_core;
    RenderEngine* m_engine;
};

template <typename T_FrameData = u8>
class System : public SystemBase {
public:
    System(RenderEngine* engine) : SystemBase(engine) {}

protected:
    T_FrameData& get_frame_data() {
        assert(m_framely_data.size() != 0 && "frame data must be initializaed first!");
        return m_framely_data[get_frame_index()];
    }

    void init_frame_datas(std::function<T_FrameData(u32)> func) {
        m_framely_data.resize(get_frame_overlap());
        for (u32 i = 0; i < get_frame_overlap(); i++) {
            m_framely_data[i] = func(i);
        }
    }

private:
    std::vector<T_FrameData> m_framely_data;
};

} // namespace vke
