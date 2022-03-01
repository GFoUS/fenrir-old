#pragma once

#include "core/core.h"
#include "vulkan/vulkan.h"

#include "physical.h"

class Instance {
public:
    Instance(const std::vector<const char*> extensions, const std::vector<const char*> validationLayers);
    ~Instance();

    std::vector<PhysicalDevice*> GetPhysicalDevices();


private:
    VkInstance instance;
    std::vector<PhysicalDevice*> physicalDevices;
};