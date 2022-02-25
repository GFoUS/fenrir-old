#pragma once

#include "core/core.h"
#include "vulkan/vulkan.h"

class Instance {
public:
    Instance(const std::vector<const char*> extensions, const std::vector<const char*> validationLayers);
    ~Instance();
private:
    VkInstance instance;

};