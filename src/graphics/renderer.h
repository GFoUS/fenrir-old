#pragma once

#include "optional"
#include "vulkan/vulkan.h"

struct QueueFamilyIndices {
    std::optional<uint32_t> graphicsFamily;

    bool isComplete() {
        return this->graphicsFamily.has_value();
    }
};

class Renderer {
public:
    Renderer();
    ~Renderer();
private:
    void CreateInstance();
    void PickPhysicalDevice();
    void MakeLogicalDevice();
    bool IsDeviceSuitable(const VkPhysicalDevice& device);
    QueueFamilyIndices GetQueueFamilies(const VkPhysicalDevice& device);

    VkInstance instance;
    VkPhysicalDevice physicalDevice;
    VkDevice device;
    VkQueue graphicsQueue;
};