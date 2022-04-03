#include "vulkan/vulkan.h"
#include <vulkan/vulkan_core.h>
#pragma once

#include "core/core.h"
#include <optional>

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

QueueFamilies FindQueueFamilies(const Context *context, const VkPhysicalDevice physicalDevice = nullptr);
SwapchainDetails GetSwapchainDetails(const Context *context, const VkPhysicalDevice physicalDevice = nullptr);
VkPhysicalDevice PickPhysicalDevice(const Context* context, const std::vector<VkPhysicalDevice> &devices, const std::vector<const char*> &requiredExtensions);