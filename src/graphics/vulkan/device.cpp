#include "device.h"
#include "physical.h"
#include "instance.h"

namespace tk
{
    VkDevice GetDevice(VkPhysicalDevice physical, VkSurfaceKHR surface)
    {
        // CREATE GRAPHICS QUEUE
        QueueFamily graphicsQueueFamily = GetGraphicsQueueFamily(physical);
        QueueFamily presentQueueFamily = GetPresentQueueFamily(physical, surface);

        std::set<uint32_t> uniqueQueueFamilies = {graphicsQueueFamily.index, presentQueueFamily.index};

        std::vector<VkDeviceQueueCreateInfo> queueInfos;
        float priority = 1.0f;

        for (auto &queueFamily : uniqueQueueFamilies)
        {
            VkDeviceQueueCreateInfo queueInfo{};
            queueInfo.flags = 0;
            queueInfo.pNext = nullptr;
            queueInfo.pQueuePriorities = &priority;
            queueInfo.queueCount = 1;
            queueInfo.queueFamilyIndex = queueFamily;
            queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;

            queueInfos.push_back(queueInfo);
        }

        // CREATE DEVICE
#ifdef FEN_DEBUG
        std::vector<const char *> validationLayers = _GetValidationLayers();
#else
        std::vector<const char *> validationLayers = {};
#endif
        std::vector<const char *> extensions = _GetDeviceExtensions(physical);

        VkDeviceCreateInfo createInfo{};
        createInfo.enabledExtensionCount = extensions.size();
        createInfo.enabledLayerCount = extensions.size();
        createInfo.flags = 0;
        createInfo.pEnabledFeatures = 0;
        createInfo.pNext = nullptr;
        createInfo.ppEnabledExtensionNames = extensions.data();
        createInfo.ppEnabledLayerNames = validationLayers.data();
        createInfo.pQueueCreateInfos = queueInfos.data();
        createInfo.queueCreateInfoCount = queueInfos.size();
        createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

        VkDevice device;
        VkResult result = vkCreateDevice(physical, &createInfo, nullptr, &device);
        if (result != VK_SUCCESS)
        {
            CRITICAL("Vulkan device creation failed with error code: {}", result);
        }

        return device;
    }

    VkQueue GetGraphicsQueue(VkPhysicalDevice physical, VkDevice device)
    {
        QueueFamily graphicsFamily = GetGraphicsQueueFamily(physical);
        VkQueue queue;
        vkGetDeviceQueue(device, graphicsFamily.index, 0, &queue);
        return queue;
    }

    VkQueue GetPresentQueue(VkPhysicalDevice physical, VkDevice device, VkSurfaceKHR surface) {
        QueueFamily presentFamily = GetPresentQueueFamily(physical, surface);
        VkQueue queue;
        vkGetDeviceQueue(device, presentFamily.index, 0, &queue);
        return queue;
    }

    std::vector<const char *> _GetDeviceExtensions(VkPhysicalDevice physical)
    {
        // DEFINE REQUIRED DEVICE EXTENSIONS HERE
        std::vector<const char *> extensions = {

        };

        uint32_t numAvailableExtensions;
        vkEnumerateDeviceExtensionProperties(physical, nullptr, &numAvailableExtensions, nullptr);
        std::vector<VkExtensionProperties> availableExtensions(numAvailableExtensions);
        vkEnumerateDeviceExtensionProperties(physical, nullptr, &numAvailableExtensions, availableExtensions.data());

        for (auto &extension : extensions)
        {
            bool found = false;

            for (auto &available : availableExtensions)
            {
                if (strcmp(extension, available.extensionName) == 0)
                {
                    INFO("Found extension: {}", extension);
                    found = true;
                }
            }

            if (!found)
            {
                CRITICAL("Required extension is not present: {}", extension);
            }
        }

        return extensions;
    }
}