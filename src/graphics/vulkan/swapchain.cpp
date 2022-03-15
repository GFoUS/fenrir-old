#include "swapchain.h"
#include "physical.h"

namespace tk
{
    Swapchain GetSwapchain(VkPhysicalDevice physical, VkSurfaceKHR surface, VkDevice device, GLFWwindow *window)
    {
        Swapchain swapchain{};
        SwapchainCapabilities capabilities = GetSwapchainCapabilities(physical, surface);

        VkSurfaceFormatKHR format = _GetSurfaceFormat(capabilities.formats);
        VkPresentModeKHR mode = _GetPresentMode(capabilities.presentModes);
        VkExtent2D extent = _GetSwapExtent(capabilities.capabilities, window);
        swapchain.extent = extent;
        swapchain.format = format;

        uint32_t imageCount = capabilities.capabilities.minImageCount + 1; // one extra so we don't get interupted by driver stuff
        if (capabilities.capabilities.maxImageCount != 0 && imageCount > capabilities.capabilities.maxImageCount)
        { // clamp image count to max image count with special rule for 0 meaning there is no max
            imageCount = capabilities.capabilities.maxImageCount;
        }

        QueueFamily graphicsFamily = GetGraphicsQueueFamily(physical);
        QueueFamily presentFamily = GetPresentQueueFamily(physical, surface);


        VkSwapchainCreateInfoKHR createInfo{};

        if (graphicsFamily.index == presentFamily.index) {
            createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        } else {
            uint32_t queueFamilies[2] = {graphicsFamily.index, presentFamily.index};
            createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
            createInfo.queueFamilyIndexCount = 2;
            createInfo.pQueueFamilyIndices = queueFamilies;
        }

        createInfo.clipped = VK_TRUE;
        createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        createInfo.flags = 0;
        createInfo.imageArrayLayers = 1;
        createInfo.imageColorSpace = format.colorSpace;
        createInfo.imageExtent = extent;
        createInfo.imageFormat = format.format;
        createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        createInfo.minImageCount = imageCount;
        createInfo.oldSwapchain = nullptr;
        createInfo.pNext = nullptr;
        createInfo.presentMode = mode;
        createInfo.preTransform = capabilities.capabilities.currentTransform;
        createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        createInfo.surface = surface;

        VkSwapchainKHR sc;
        VkResult result = vkCreateSwapchainKHR(device, &createInfo, nullptr, &sc);
        if (result != VK_SUCCESS) {
            CRITICAL("Swapchain creation failed with error code: {}", result);
        }
        swapchain.swapchain = sc;

        return swapchain;
    }

    std::vector<VkImage> GetSwapchainImages(VkDevice device, VkSwapchainKHR swapchain) {
        uint32_t numImages;
        vkGetSwapchainImagesKHR(device, swapchain, &numImages, nullptr);
        std::vector<VkImage> images(numImages);
        vkGetSwapchainImagesKHR(device, swapchain, &numImages, images.data());
        return images;
    }

    std::vector<VkImageView> GetSwapchainImageViews(VkDevice device, std::vector<VkImage> images, VkSurfaceFormatKHR format) {
        std::vector<VkImageView> imageViews(images.size());

        for (uint32_t i = 0; i < imageViews.size(); i++) {
            VkImageViewCreateInfo createInfo{};
            createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.flags = 0;
            createInfo.format = format.format;
            createInfo.image = images[i];
            createInfo.pNext = nullptr;
            createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            createInfo.subresourceRange.baseArrayLayer = 0;
            createInfo.subresourceRange.baseMipLevel = 0;
            createInfo.subresourceRange.layerCount = 1;
            createInfo.subresourceRange.levelCount = 1;
            createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;

            VkResult result = vkCreateImageView(device, &createInfo, nullptr, &imageViews[i]);
            if (result != VK_SUCCESS) {
                CRITICAL("Image view creation failed with error code: {}", result);
            }
        }

        return imageViews;
    }


    SwapchainCapabilities GetSwapchainCapabilities(VkPhysicalDevice physical, VkSurfaceKHR surface)
    {
        SwapchainCapabilities capabilities;

        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical, surface, &capabilities.capabilities);

        uint32_t numFormats;
        vkGetPhysicalDeviceSurfaceFormatsKHR(physical, surface, &numFormats, nullptr);
        capabilities.formats.resize(numFormats);
        vkGetPhysicalDeviceSurfaceFormatsKHR(physical, surface, &numFormats, capabilities.formats.data());

        uint32_t numPresentModes;
        vkGetPhysicalDeviceSurfacePresentModesKHR(physical, surface, &numPresentModes, nullptr);
        capabilities.presentModes.resize(numPresentModes);
        vkGetPhysicalDeviceSurfacePresentModesKHR(physical, surface, &numPresentModes, capabilities.presentModes.data());

        return capabilities;
    }

    VkSurfaceFormatKHR _GetSurfaceFormat(std::vector<VkSurfaceFormatKHR> formats)
    {
        for (auto &format : formats)
        {
            if (format.format == VK_FORMAT_B8G8R8A8_SRGB && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
            {
                return format;
            }
        }

        return formats[0];
    }

    VkPresentModeKHR _GetPresentMode(std::vector<VkPresentModeKHR> modes)
    {
        return VK_PRESENT_MODE_FIFO_KHR;
    }

    VkExtent2D _GetSwapExtent(VkSurfaceCapabilitiesKHR capabilities, GLFWwindow *window)
    {
        if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
        {
            return capabilities.currentExtent;
        }
        else
        {
            int width, height;
            glfwGetFramebufferSize(window, &width, &height);

            VkExtent2D actualExtent = {
                static_cast<uint32_t>(width),
                static_cast<uint32_t>(height)};

            actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
            actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

            return actualExtent;
        }
    }
}