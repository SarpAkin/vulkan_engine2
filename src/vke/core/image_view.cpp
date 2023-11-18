#include "image_view.hpp"

namespace vke {

ImageView::ImageView(Image* image, const VkImageViewCreateInfo& c_info) : Resource(image->core()) {
    m_vke_image = image;

    m_subresource_range = c_info.subresourceRange;
    m_view_type         = c_info.viewType;

    vkCreateImageView(device(), &c_info, nullptr, &m_view);
}

ImageView::~ImageView() {
    vkDestroyImageView(device(), m_view, nullptr);
}
} // namespace vke
