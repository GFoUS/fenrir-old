#include "instance.h"

#include "GLFW/glfw3.h"

Instance::Instance(const std::vector<const char*> extensions, const std::vector<const char*> validationLayers) {
    // APP INFO
    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Fenrir";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "Fenrir";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_3;

    // BASE CREATE INFO
    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;

    // EXTENSIONS
    uint32_t numAvailableExtensions = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &numAvailableExtensions, nullptr);
    std::vector<VkExtensionProperties> availableExtensions(numAvailableExtensions);
    vkEnumerateInstanceExtensionProperties(nullptr, &numAvailableExtensions, availableExtensions.data());

    // Check that required extensions are present
    for (const auto& extension : extensions) {
        bool found = false;

        for (const auto& available : availableExtensions) {
            if (!strcmp(available.extensionName, extension)) {
                found = true;
                DEBUG("Found extension {}", extension);
            }
        }

        if (!found) {
            CRITICAL("Instance extension with name {} could not be found", extension);
        }
    }

    createInfo.enabledExtensionCount = extensions.size();
    createInfo.ppEnabledExtensionNames = extensions.data();

    // VALIDATION LAYERS
    bool enableLayers;

    #if FEN_DEBUG
        enableLayers = true;
    #else
        enableLayers = false;
    #endif

    uint32_t numAvailableLayers = 0;
    vkEnumerateInstanceLayerProperties(&numAvailableLayers, nullptr);
    std::vector<VkLayerProperties> availableLayers(numAvailableLayers);
    vkEnumerateInstanceLayerProperties(&numAvailableLayers, availableLayers.data());

    if (enableLayers) {
        for (const char* layer : validationLayers) {
            bool found = false;

            for (const VkLayerProperties& available : availableLayers) {
                if (!strcmp(available.layerName, layer)) {
                    found = true;
                    DEBUG("Found validation layer: {}", layer);
                }
            }

            if (!found) {
                WARN("Validation layer with name {} could not be found, no validation layers will be enabled", layer);
                break;
            }
        }

        createInfo.enabledLayerCount = validationLayers.size();
        createInfo.ppEnabledLayerNames = validationLayers.data();
    }

    if (createInfo.ppEnabledLayerNames == nullptr) {
        createInfo.enabledLayerCount = 0;
    }

    // CREATE INSTANCE
    VkResult result = vkCreateInstance(&createInfo, nullptr, &this->instance);
    if (result != VK_SUCCESS) {
        CRITICAL("Vulkan instance creation failed with error code: {}", result);
    } 

    // PHYSICAL DEVICES
    uint32_t numPhysicalDevices;
    vkEnumeratePhysicalDevices(this->instance, &numPhysicalDevices, nullptr);
    std::vector<VkPhysicalDevice> physicalDevicesRaw(numPhysicalDevices);
    vkEnumeratePhysicalDevices(this->instance, &numPhysicalDevices, physicalDevicesRaw.data());

    this->physicalDevices.resize(numPhysicalDevices);
    for (auto& physicalDeviceRaw : physicalDevicesRaw) {
        this->physicalDevices.push_back(new PhysicalDevice(physicalDeviceRaw));

        #ifdef FEN_DEBUG
            VkPhysicalDeviceProperties properties;
            vkGetPhysicalDeviceProperties(physicalDeviceRaw, &properties);
            INFO("Found device: {}", properties.deviceName);
        #endif
    }
}

Instance::~Instance() {
    vkDestroyInstance(this->instance, nullptr);

    for (auto physicalDevice : this->physicalDevices) {
        delete physicalDevice;
    }
}

std::vector<PhysicalDevice*> Instance::GetPhysicalDevices() {
    return this->physicalDevices;
}