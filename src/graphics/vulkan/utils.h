#pragma once

#include "vulkan/vulkan.h"
#include "core/core.h"
#include "vertex.h"

struct Context;

struct QueueFamilies {
    std::optional<uint32_t> graphics;
    std::optional<uint32_t> present;

    bool Complete() {
        return this->graphics.has_value() && this->present.has_value();
    }

    std::set<uint32_t> GetUniques() {
        return {this->graphics.value(), this->present.value()};
    }
};

struct SwapchainDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

QueueFamilies FindQueueFamilies(const Context *context, VkPhysicalDevice physicalDevice = nullptr);
SwapchainDetails GetSwapchainDetails(const Context *context, VkPhysicalDevice physicalDevice = nullptr);
VkSampleCountFlagBits GetMaxUsableSampleCount(VkPhysicalDeviceProperties physicalDeviceProperties);
VkPhysicalDevice PickPhysicalDevice(const Context* context, const std::vector<VkPhysicalDevice> &devices, const std::vector<const char*> &requiredExtensions);

void CreateBuffer(const Context* context, VkBuffer& buffer, VkDeviceMemory& bufferMemory, VkDeviceSize size, VkMemoryPropertyFlags requiredProperties, VkBufferUsageFlags usage);
uint32_t Pad(uint32_t value, uint32_t align);


