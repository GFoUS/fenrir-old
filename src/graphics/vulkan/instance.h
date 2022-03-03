#pragma once

#include "core/core.h"
#include "vulkan/vulkan.h"

namespace tk
{
    VkInstance GetInstance();
    std::vector<const char *> _GetExtensions();
    std::vector<const char *> _GetValidationLayers();
}