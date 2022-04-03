#include "utils.h"
#include "context.h"
#include <vulkan/vulkan_core.h>

QueueFamilies FindQueueFamilies(const Context *context, const VkPhysicalDevice physicalDevice) {
    QueueFamilies families;
    VkPhysicalDevice physical = physicalDevice ? physicalDevice : context->physical;

    uint32_t numQueueFamilies;
    vkGetPhysicalDeviceQueueFamilyProperties(physical, &numQueueFamilies, nullptr);
    std::vector<VkQueueFamilyProperties> queueFamilies(numQueueFamilies);
    vkGetPhysicalDeviceQueueFamilyProperties(physical, &numQueueFamilies, queueFamilies.data());

    for (uint32_t i = 0; i < numQueueFamilies; i++) {
        if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            families.graphics = i;
        }
        VkBool32 supportsPresent = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(physical, i, context->surface, &supportsPresent);
        if (supportsPresent) {
            families.present = i;
        }

        if (families.Complete()) return families;
    }

    return families;
}

SwapchainDetails GetSwapchainDetails(const Context *context, const VkPhysicalDevice physicalDevice) {
    auto physical = physicalDevice ? physicalDevice : context->physical;
    SwapchainDetails details{};

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical, context->surface, &details.capabilities);

    uint32_t numFormats;
    vkGetPhysicalDeviceSurfaceFormatsKHR(physical, context->surface, &numFormats, nullptr);
    details.formats.resize(numFormats);
    vkGetPhysicalDeviceSurfaceFormatsKHR(physical, context->surface, &numFormats, details.formats.data());

    uint32_t numPresentModes;
    vkGetPhysicalDeviceSurfacePresentModesKHR(physical, context->surface, &numPresentModes, nullptr);
    details.presentModes.resize(numPresentModes);
    vkGetPhysicalDeviceSurfacePresentModesKHR(physical, context->surface, &numPresentModes, details.presentModes.data());

    return details;
}

VkPhysicalDevice PickPhysicalDevice(const Context* context, const std::vector<VkPhysicalDevice> &devices, const std::vector<const char*> &requiredExtensions) {
    for (const auto& physical : devices) {
        QueueFamilies families = FindQueueFamilies(context, physical);

        // Check for extensions
        bool requiredExtensionsPresent = true;
        uint32_t numAvailableDeviceExtensions;
        vkEnumerateDeviceExtensionProperties(physical, nullptr, &numAvailableDeviceExtensions, nullptr);
        std::vector<VkExtensionProperties> availableDeviceExtensions(numAvailableDeviceExtensions);
        vkEnumerateDeviceExtensionProperties(physical, nullptr, &numAvailableDeviceExtensions, availableDeviceExtensions.data());

        for (const auto& extension : requiredExtensions) {
            bool found = false;
            for (const auto& available : availableDeviceExtensions) {
                if (strcmp(available.extensionName, extension) == 0) {
                    found = true;
                }
            }
            
            if (!found) {
                requiredExtensionsPresent = false;
            }
        }

        // Check swapchain support
        SwapchainDetails details = GetSwapchainDetails(context, physical);
        bool swapChainSupported = !details.formats.empty() && !details.presentModes.empty();

        if (families.Complete() && requiredExtensionsPresent && swapChainSupported) {
            return physical;
        }
    }

    CRITICAL("No suitable devices found!");
    return nullptr;
}