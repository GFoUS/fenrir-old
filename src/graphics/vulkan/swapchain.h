#pragma once

#include "core/core.h"
#include "vulkan/vulkan.h"
#define GLFW_INCLUDE_VULKAN
#include "GLFW/glfw3.h"

namespace tk
{
    struct SwapchainCapabilities
    {
        VkSurfaceCapabilitiesKHR capabilities;
        std::vector<VkSurfaceFormatKHR> formats;
        std::vector<VkPresentModeKHR> presentModes;
    };

    struct Swapchain {
        VkSwapchainKHR swapchain;
        VkSurfaceFormatKHR format;
        VkExtent2D extent;
    };

    Swapchain GetSwapchain(VkPhysicalDevice physical, VkSurfaceKHR surface, VkDevice device, GLFWwindow* window);
    std::vector<VkImage> GetSwapchainImages(VkDevice device, VkSwapchainKHR swapchain);
    std::vector<VkImageView> GetSwapchainImageViews(VkDevice device, std::vector<VkImage> images, VkSurfaceFormatKHR format);
    SwapchainCapabilities GetSwapchainCapabilities(VkPhysicalDevice physical, VkSurfaceKHR surface);

    VkSurfaceFormatKHR _GetSurfaceFormat(std::vector<VkSurfaceFormatKHR> formats);
    VkPresentModeKHR _GetPresentMode(std::vector<VkPresentModeKHR> modes);
    VkExtent2D _GetSwapExtent(VkSurfaceCapabilitiesKHR capabilities, GLFWwindow* window);
}