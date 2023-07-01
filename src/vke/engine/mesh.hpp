#pragma once

#include <memory>

#include "../common.hpp"
#include "../fwd.hpp"

namespace vke {

class Mesh {
public:
private:
    std::unique_ptr<Buffer> m_indicies;
    std::unique_ptr<Buffer> m_vpos, m_vuvs, m_vnorm;
};

} // namespace vke