#pragma once

#include "core/core.h"
#include "vulkan/vulkan.h"
#include "swapchain.h"

namespace tk
{
    VkRenderPass GetRenderPass(VkDevice device, Swapchain swapchain);
}