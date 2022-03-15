#pragma once

#include "core/core.h"
#include "vulkan/vulkan.h"

namespace tk
{
    struct QueueFamily
    {
        uint32_t index;
        VkQueueFamilyProperties *properties;
    };

    VkPhysicalDevice PickPhysical(VkInstance instance, VkSurfaceKHR surface);
    QueueFamily GetGraphicsQueueFamily(VkPhysicalDevice physical);
    QueueFamily GetPresentQueueFamily(VkPhysicalDevice physical, VkSurfaceKHR surface);

    bool _HasGraphicsQueue(VkPhysicalDevice physical);
}
