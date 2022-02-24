#include "renderer.h"

#include "core/core.h"
#include "GLFW/glfw3.h"

Renderer::Renderer() {
    CreateInstance();
}

Renderer::~Renderer() {
    vkDestroyInstance(this->instance, nullptr);
}

void Renderer::CreateInstance() {
    // APP INFO
    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Fenrir";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "Fenrir";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_1;

    // BASE CREATE INFO
    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;

    // EXTENSIONS
    uint32_t numExtensions = 0;
    const char** extensions = glfwGetRequiredInstanceExtensions(&numExtensions);

    uint32_t numAvailableExtensions = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &numAvailableExtensions, nullptr);
    std::vector<VkExtensionProperties> availableExtensions(numAvailableExtensions);
    vkEnumerateInstanceExtensionProperties(nullptr, &numAvailableExtensions, availableExtensions.data());

    // Check that required extensions are present
    for (uint32_t i = 0; i < numExtensions; i++) {
        bool found = false;

        for (const auto& available : availableExtensions) {
            if (!strcmp(available.extensionName, extensions[i])) {
                found = true;
                DEBUG("Found extension {}", extensions[i]);
            }
        }

        if (!found) {
            CRITICAL("Instance extension with name {} could not be found", extensions[i]);
        }
    }

    createInfo.enabledExtensionCount = numExtensions;
    createInfo.ppEnabledExtensionNames = extensions;

    // VALIDATION LAYERS
    std::vector<const char*> validationLayers = {
        "VK_LAYER_KHRONOS_validation",
        "VK_LAYER_LUNARG_api_dump"
    };

    uint32_t numAvailableLayers = 0;
    vkEnumerateInstanceLayerProperties(&numAvailableLayers, nullptr);
    std::vector<VkLayerProperties> availableLayers(numAvailableLayers);
    vkEnumerateInstanceLayerProperties(&numAvailableLayers, availableLayers.data());

    for (const char* layer : validationLayers) {
        bool found = false;

        for (const VkLayerProperties& available : availableLayers) {
            if (!strcmp(available.layerName, layer)) {
                found = true;
                DEBUG("Found validation layer: {}", layer);
            }
        }

        if (!found) {
            WARN("Validation layer with name {} could not be found", layer);
        }
    }

    createInfo.enabledLayerCount = validationLayers.size();
    createInfo.ppEnabledLayerNames = validationLayers.data();

    // CREATE INSTANCE
    VkResult result = vkCreateInstance(&createInfo, nullptr, &this->instance);
    if (result != VK_SUCCESS) {
        CRITICAL("Vulkan instance creation failed with error code: {}", result);
    }  
}