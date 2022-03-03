#include "physical.h"

namespace tk
{
    VkPhysicalDevice PickPhysical(VkInstance instance)
    {
        uint32_t numPhysicalDevices;
        vkEnumeratePhysicalDevices(instance, &numPhysicalDevices, nullptr);
        std::vector<VkPhysicalDevice> physicalDevices(numPhysicalDevices);
        vkEnumeratePhysicalDevices(instance, &numPhysicalDevices, physicalDevices.data());

        for (auto &physical : physicalDevices)
        {
            VkPhysicalDeviceProperties properties;
            vkGetPhysicalDeviceProperties(physical, &properties);

            if (_HasGraphicsQueue(physical))
            {
                INFO("Picking device: {}", properties.deviceName);
                return physical;
            }
        }

        CRITICAL("No suitable devices found");
    }

    QueueFamily GetGraphicsQueueFamily(VkPhysicalDevice physical)
    {
        uint32_t numQueueFamilies;
        vkGetPhysicalDeviceQueueFamilyProperties(physical, &numQueueFamilies, nullptr);
        std::vector<VkQueueFamilyProperties> queueFamilies(numQueueFamilies);
        vkGetPhysicalDeviceQueueFamilyProperties(physical, &numQueueFamilies, queueFamilies.data());

        for (uint32_t i = 0; i < queueFamilies.size(); i++)
        {
            if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
            {
                return {i, &queueFamilies[i]};
            }
        }

        return {0, nullptr};
    }

    QueueFamily GetPresentQueueFamily(VkPhysicalDevice physical, VkSurfaceKHR surface)
    {
        uint32_t numQueueFamilies;
        vkGetPhysicalDeviceQueueFamilyProperties(physical, &numQueueFamilies, nullptr);
        std::vector<VkQueueFamilyProperties> queueFamilies(numQueueFamilies);
        vkGetPhysicalDeviceQueueFamilyProperties(physical, &numQueueFamilies, queueFamilies.data());

        for (uint32_t i = 0; i < queueFamilies.size(); i++)
        {
            VkBool32 presentSupport = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(physical, i, surface, &presentSupport);
            if (presentSupport)
            {
                return {i, &queueFamilies[i]};
            }
        }

        return {0, nullptr};
    }

    bool _HasGraphicsQueue(VkPhysicalDevice physical)
    {
        return GetGraphicsQueueFamily(physical).properties != nullptr;
    }
}
