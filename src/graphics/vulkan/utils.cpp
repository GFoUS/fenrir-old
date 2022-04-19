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

        VkPhysicalDeviceFeatures features;
        vkGetPhysicalDeviceFeatures(physical, &features);

        if (families.Complete() && requiredExtensionsPresent && swapChainSupported && features.samplerAnisotropy) {
            return physical;
        }
    }

    CRITICAL("No suitable devices found!");
    return nullptr;
}

void CreateBuffer(const Context* context, VkBuffer& buffer, VkDeviceMemory& bufferMemory, VkDeviceSize size, VkMemoryPropertyFlags requiredProperties, VkBufferUsageFlags usage) {
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VkResult bufferResult = vkCreateBuffer(context->device, &bufferInfo, nullptr, &buffer);
    if (bufferResult != VK_SUCCESS) {
        CRITICAL("Vertex buffer creation failed with error code: {}", bufferResult);
    }

    VkMemoryRequirements requirements;
    vkGetBufferMemoryRequirements(context->device, buffer, &requirements);
    VkPhysicalDeviceMemoryProperties properties;
    vkGetPhysicalDeviceMemoryProperties(context->physical, &properties);

    uint32_t memoryTypeIndex;
    for (uint32_t i = 0; i < properties.memoryTypeCount; i++) {
        if ((requirements.memoryTypeBits & (1 << i)) && (properties.memoryTypes[i].propertyFlags & requiredProperties) == requiredProperties) {
            memoryTypeIndex = i;
            break;
        }
        if (i == properties.memoryTypeCount - 1) {
            CRITICAL("No suitable memory types found for type {} and properties {}", requirements.memoryTypeBits, requiredProperties);
        }
    }

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = requirements.size;
    allocInfo.memoryTypeIndex = memoryTypeIndex;

    VkResult allocResult = vkAllocateMemory(context->device, &allocInfo, nullptr, &bufferMemory);
    if (allocResult != VK_SUCCESS) {
        CRITICAL("Allocation failed with error code: {}", allocResult);
    }
    vkBindBufferMemory(context->device, buffer, bufferMemory, 0);
}


uint32_t Pad(uint32_t value, uint32_t align) {
    if (align > 0) {
        return (value + align - 1) & ~(align - 1);
    }
    return value;
}


VkSampleCountFlagBits GetMaxUsableSampleCount(VkPhysicalDeviceProperties physicalDeviceProperties) {
    VkSampleCountFlags counts = physicalDeviceProperties.limits.framebufferColorSampleCounts & physicalDeviceProperties.limits.framebufferDepthSampleCounts;
    if (counts & VK_SAMPLE_COUNT_64_BIT) { return VK_SAMPLE_COUNT_64_BIT; }
    if (counts & VK_SAMPLE_COUNT_32_BIT) { return VK_SAMPLE_COUNT_32_BIT; }
    if (counts & VK_SAMPLE_COUNT_16_BIT) { return VK_SAMPLE_COUNT_16_BIT; }
    if (counts & VK_SAMPLE_COUNT_8_BIT) { return VK_SAMPLE_COUNT_8_BIT; }
    if (counts & VK_SAMPLE_COUNT_4_BIT) { return VK_SAMPLE_COUNT_4_BIT; }
    if (counts & VK_SAMPLE_COUNT_2_BIT) { return VK_SAMPLE_COUNT_2_BIT; }

    return VK_SAMPLE_COUNT_1_BIT;
}