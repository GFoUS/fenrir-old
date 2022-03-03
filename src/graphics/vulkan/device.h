#pragma once

#include "core/core.h"
#include "vulkan/vulkan.h"

namespace tk
{
    VkDevice GetDevice(VkPhysicalDevice physical, VkSurfaceKHR surface);
    VkQueue GetGraphicsQueue(VkPhysicalDevice physical, VkDevice device);
    VkQueue GetPresentQueue(VkPhysicalDevice physical, VkDevice device, VkSurfaceKHR surface);

    std::vector<const char *> _GetDeviceExtensions(VkPhysicalDevice physical);
}