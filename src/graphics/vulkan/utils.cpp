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

void CopyBuffer(const Context* context, VkBuffer src, VkBuffer dst, VkDeviceSize size) {
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = context->commandPool;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    VkResult allocResult = vkAllocateCommandBuffers(context->device, &allocInfo, &commandBuffer);
    if (allocResult != VK_SUCCESS) {
        CRITICAL("Command buffer alocation during copy failed with error code: {}", allocResult);
    }
    
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    VkResult beginResult = vkBeginCommandBuffer(commandBuffer, &beginInfo);
    if (beginResult != VK_SUCCESS) {
        CRITICAL("Beginning copy command buffer failed with error code: {}", beginResult);
    }

    VkBufferCopy copy{};
    copy.srcOffset = 0;
    copy.dstOffset = 0;
    copy.size = size;
    vkCmdCopyBuffer(commandBuffer, src, dst, 1, &copy);

    VkResult endResult = vkEndCommandBuffer(commandBuffer);
    if (endResult != VK_SUCCESS) {
        CRITICAL("Ending copy command buffer failed with error code: {}", endResult);
    }

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    VkResult submitResult = vkQueueSubmit(context->graphics, 1, &submitInfo, VK_NULL_HANDLE);
    if (submitResult != VK_SUCCESS) {
        CRITICAL("Submitting copy command buffer failed with error code: {}", submitResult);
    }
    vkQueueWaitIdle(context->graphics);

    vkFreeCommandBuffers(context->device, context->commandPool, 1, &commandBuffer);
}