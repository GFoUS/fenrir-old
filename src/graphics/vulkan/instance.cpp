#include "instance.h"
#include "GLFW/glfw3.h"
namespace tk
{
    VkInstance GetInstance()
    {
        VkApplicationInfo appInfo{};
        appInfo.apiVersion = VK_API_VERSION_1_3;
        appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.pApplicationName = "fenrir";
        appInfo.pEngineName = "fenrir";
        appInfo.pNext = nullptr;
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;

        std::vector<const char *> extensions = _GetExtensions();
#ifdef FEN_DEBUG
        std::vector<const char *> validationLayers = _GetValidationLayers();
#else
        std::vector<const char *> validationLayers;
#endif

        VkInstanceCreateInfo createInfo{};
        createInfo.enabledExtensionCount = extensions.size();
        createInfo.enabledLayerCount = validationLayers.size();
        createInfo.flags = 0;
        createInfo.pApplicationInfo = &appInfo;
        createInfo.pNext = nullptr;
        createInfo.ppEnabledExtensionNames = extensions.data();
        createInfo.ppEnabledLayerNames = validationLayers.data();
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;

        VkInstance instance;
        VkResult result = vkCreateInstance(&createInfo, nullptr, &instance);
        if (result != VK_SUCCESS)
        {
            CRITICAL("Vulkan instance creation failed");
        }

        return instance;
    }

    std::vector<const char *> _GetExtensions()
    {
        // DEFINE REQUIRED EXTENSIONS HERE
        std::vector<const char *> extensions = {

        };

        uint32_t numGlfwExtensions;
        const char **glfwExtensions = glfwGetRequiredInstanceExtensions(&numGlfwExtensions);

        for (uint32_t i = 0; i < numGlfwExtensions; i++)
        {
            extensions.push_back(glfwExtensions[i]);
        }

        uint32_t numAvailableExtensions;
        vkEnumerateInstanceExtensionProperties(nullptr, &numAvailableExtensions, nullptr);
        std::vector<VkExtensionProperties> availableExtensions(numAvailableExtensions);
        vkEnumerateInstanceExtensionProperties(nullptr, &numAvailableExtensions, availableExtensions.data());

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

    std::vector<const char *> _GetValidationLayers()
    {
        std::vector<const char *> validationLayers = {
            "VK_LAYER_KHRONOS_validation"};

        uint32_t numAvailableLayers;
        vkEnumerateInstanceLayerProperties(&numAvailableLayers, nullptr);
        std::vector<VkLayerProperties> availableLayers(numAvailableLayers);
        vkEnumerateInstanceLayerProperties(&numAvailableLayers, availableLayers.data());

        for (auto &layer : validationLayers)
        {
            bool found = false;

            for (auto &available : availableLayers)
            {
                if (strcmp(layer, available.layerName) == 0)
                {
                    INFO("Found validation layer: {}", layer);
                    found = true;
                }
            }

            if (!found)
            {
                WARN("Validation layer is not present: {}, no validation layers will be activated", layer);
                return {};
            }
        }

        return validationLayers;
    }
}
