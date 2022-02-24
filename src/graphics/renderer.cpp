#include "renderer.h"

#include "core/core.h"
#include "GLFW/glfw3.h"

Renderer::Renderer() {
    CreateInstance();
    INFO("Vulkan instance created");
    PickPhysicalDevice();
    INFO("Physical device found");
    MakeLogicalDevice();
    INFO("Logical device made");
}

Renderer::~Renderer() {
    vkDestroyDevice(this->device, nullptr);
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
        "VK_LAYER_KHRONOS_validation"
    };

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
                WARN("Validation layer with name {} could not be found", layer);
                enableLayers = false;
            }
        }
    }
    
    // Second check is needed incase validation layers couldn't be found
    if (enableLayers) {
        createInfo.enabledLayerCount = validationLayers.size();
        createInfo.ppEnabledLayerNames = validationLayers.data();
    } else {
        createInfo.enabledLayerCount = 0;
    }

    // CREATE INSTANCE
    VkResult result = vkCreateInstance(&createInfo, nullptr, &this->instance);
    if (result != VK_SUCCESS) {
        CRITICAL("Vulkan instance creation failed with error code: {}", result);
    } 
}

void Renderer::PickPhysicalDevice() {
    // GET ALL DEVICES
    uint32_t numDevices = 0;
    vkEnumeratePhysicalDevices(this->instance, &numDevices, nullptr);
    if (numDevices == 0) {
        CRITICAL("No GPUs that support vulkan are present");
    }
    std::vector<VkPhysicalDevice> devices(numDevices);
    vkEnumeratePhysicalDevices(this->instance, &numDevices, devices.data());

    // CHECK DEVICE SUITABILITY
    for (const auto& device : devices) {
        if (IsDeviceSuitable(device)) {
            this->physicalDevice = device;
            break;
        }
    }

    if (this->physicalDevice == nullptr) {
        CRITICAL("No device is suitable");
    }
}

bool Renderer::IsDeviceSuitable(const VkPhysicalDevice& device) {
    // DEVICE PROPERTIES AND FEATURES
    VkPhysicalDeviceProperties properties;
    VkPhysicalDeviceFeatures features;
    vkGetPhysicalDeviceProperties(device, &properties);
    vkGetPhysicalDeviceFeatures(device, &features);

    // DEVICE QUEUE FAMILIES
    QueueFamilyIndices queueFamilies = GetQueueFamilies(device);
   

    // TODO: Check for requirements and make scoring system?

    return queueFamilies.isComplete();
}

QueueFamilyIndices Renderer::GetQueueFamilies(const VkPhysicalDevice& device) {
    QueueFamilyIndices queueFamilyIndices;

    uint32_t numQueueFamilies;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &numQueueFamilies, nullptr);
    std::vector<VkQueueFamilyProperties> queueFamilies(numQueueFamilies);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &numQueueFamilies, queueFamilies.data());

    bool queueFound = false;
    uint32_t selectedQueueFamily;

    uint32_t i = 0;
    for (auto& queueFamily : queueFamilies) {
        if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            queueFamilyIndices.graphicsFamily = i;
        }

        i++;
    }

    return queueFamilyIndices;
}

void Renderer::MakeLogicalDevice() {
    QueueFamilyIndices queueFamilyIndices = GetQueueFamilies(this->physicalDevice);

    VkDeviceQueueCreateInfo queueCreateInfo{};
    queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();
    queueCreateInfo.queueCount = 1;
    float queuePriority = 1.0;
    queueCreateInfo.pQueuePriorities = &queuePriority;

    VkPhysicalDeviceFeatures features{};

    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.pQueueCreateInfos = &queueCreateInfo;
    createInfo.queueCreateInfoCount = 1;
    createInfo.pEnabledFeatures = &features;
    createInfo.enabledExtensionCount = 0;
    createInfo.enabledLayerCount = 0;

    if (vkCreateDevice(this->physicalDevice, &createInfo, nullptr, &this->device) != VK_SUCCESS) {
        CRITICAL("Vulkan device creation failed");
    }

    vkGetDeviceQueue(this->device, queueFamilyIndices.graphicsFamily.value(), 0, &this->graphicsQueue);
}