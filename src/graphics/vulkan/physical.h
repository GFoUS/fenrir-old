#pragma once

#include "vulkan/vulkan.h"

class PhysicalDevice {
public:
    PhysicalDevice(VkPhysicalDevice physical) : physical(physical) {};
    ~PhysicalDevice() {};

    bool GetIsSuitable();
private:
    VkPhysicalDevice physical;
};