#pragma once

#include "core/core.h"
#include "vulkan/vulkan.h"

namespace tk {
    std::vector<VkFramebuffer> GetFramebuffers(VkDevice device, std::vector<VkImageView> imageViews, VkRenderPass renderPass, VkExtent2D extent);
}