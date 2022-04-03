#include "context.h"
#include <vulkan/vulkan_core.h>
#define GLFW_INCLUDE_VULKAN
#include "GLFW/glfw3.h"
#include "graphics/vulkan/utils.h"

const std::vector<const char*> instanceExtensions = {};
const std::vector<const char*> validationLayers = {"VK_LAYER_KHRONOS_validation"};
const std::vector<const char*> deviceExtensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

Context::Context(Window* window) : window(window) {
    this->CreateDevice();
    this->CreateSwapchain();
}

Context::~Context() {
    for (auto& imageView : this->imageViews) {
        vkDestroyImageView(this->device, imageView, nullptr);
    }
    vkDestroySwapchainKHR(this->device, this->swapchain, nullptr);
    vkDestroyDevice(this->device, nullptr);
    vkDestroySurfaceKHR(this->instance, this->surface, nullptr);
    vkDestroyInstance(this->instance, nullptr);
}

void Context::CreateDevice() {
    // Create Instance
    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Fenrir";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "Fenrir";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;

    uint32_t numAvailableInstanceExtensions;
    vkEnumerateInstanceExtensionProperties(nullptr, &numAvailableInstanceExtensions, nullptr);
    std::vector<VkExtensionProperties> availableInstanceExtensions(numAvailableInstanceExtensions);
    vkEnumerateInstanceExtensionProperties(nullptr, &numAvailableInstanceExtensions, availableInstanceExtensions.data());
    uint32_t numAvailableLayers;
    vkEnumerateInstanceLayerProperties(&numAvailableLayers, nullptr);
    std::vector<VkLayerProperties> availableLayers(numAvailableLayers);
    vkEnumerateInstanceLayerProperties(&numAvailableLayers, availableLayers.data());

    std::vector<const char*> extensions = instanceExtensions;
    uint32_t numGlfwExtensions;
    const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&numGlfwExtensions);
    for (uint32_t i = 0; i < numGlfwExtensions; i++) {
        extensions.push_back(glfwExtensions[i]);
    } 

    for (const auto& extension : extensions) {
        bool found = false;
        for (const auto& available : availableInstanceExtensions) {
            if (strcmp(available.extensionName, extension) == 0) {
                found = true;
            }
        }
        
        if (!found) {
            CRITICAL("Missing instance extension: {}", extension);
        }
    }

    bool usingValidationLayers = true;
    for (const auto& layer : validationLayers) {
        bool found = false;
        for (const auto& available : availableLayers) {
            if (strcmp(available.layerName, layer) == 0) {
                found = true;
            }
        }
        
        if (!found) {
            WARN("Missing validation layer: {}", layer);
            usingValidationLayers = true;
        }
    }

    VkInstanceCreateInfo instanceInfo{};
    instanceInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instanceInfo.pApplicationInfo = &appInfo;
    instanceInfo.enabledExtensionCount = extensions.size();
    instanceInfo.enabledLayerCount = usingValidationLayers ? validationLayers.size() : 0;
    instanceInfo.ppEnabledExtensionNames = extensions.data();
    instanceInfo.ppEnabledLayerNames = usingValidationLayers ? validationLayers.data() : nullptr;
    
    VkResult instanceResult = vkCreateInstance(&instanceInfo, nullptr, &this->instance);
    if (instanceResult != VK_SUCCESS) {
        CRITICAL("Failed to create instance with error code: {}", instanceResult);
    }
    INFO("Vulkan instance created");

    // Create surface
    VkResult surfaceResult = glfwCreateWindowSurface(this->instance, this->window->GetRawWindow(), nullptr, &this->surface);
    if (surfaceResult != VK_SUCCESS) {
        CRITICAL("Surface creation failed with error code: {}", surfaceResult);
    }
    INFO("Vulkan surface created");   

    // Find physical device
    uint32_t numDevices;
    vkEnumeratePhysicalDevices(this->instance, &numDevices, nullptr);
    std::vector<VkPhysicalDevice> physicalDevices(numDevices);
    vkEnumeratePhysicalDevices(this->instance, &numDevices, physicalDevices.data());

    this->physical = PickPhysicalDevice(this, physicalDevices, deviceExtensions);
    this->queueFamilies = FindQueueFamilies(this);

    // Create device
    float queuePriority = 1.0f;
    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    for (uint32_t queueFamily : this->queueFamilies.GetUniques()) {
        VkDeviceQueueCreateInfo queueCreateInfo{};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.queueFamilyIndex = queueFamily;
        queueCreateInfo.pQueuePriorities = &queuePriority;

        queueCreateInfos.push_back(queueCreateInfo);
    }
    
    VkPhysicalDeviceFeatures deviceFeatures{};

    VkDeviceCreateInfo deviceInfo{};
    deviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceInfo.pQueueCreateInfos = queueCreateInfos.data();
    deviceInfo.queueCreateInfoCount = queueCreateInfos.size();
    deviceInfo.pEnabledFeatures = &deviceFeatures;
    deviceInfo.enabledLayerCount = usingValidationLayers ? validationLayers.size() : 0;
    deviceInfo.ppEnabledLayerNames = usingValidationLayers ? validationLayers.data() : nullptr;
    deviceInfo.enabledExtensionCount = deviceExtensions.size();
    deviceInfo.ppEnabledExtensionNames = deviceExtensions.data();

    VkResult deviceResult = vkCreateDevice(this->physical, &deviceInfo, nullptr, &this->device);
    if (deviceResult != VK_SUCCESS) {
        CRITICAL("Device create failed with error code: {}", deviceResult);
    }
    INFO("Vulkan device created");

    vkGetDeviceQueue(this->device, this->queueFamilies.graphics.value(), 0, &this->graphics);
    vkGetDeviceQueue(this->device, this->queueFamilies.present.value(), 0, &this->present);
}   

void Context::CreateSwapchain() {
    SwapchainDetails details = GetSwapchainDetails(this);

    bool formatFound = false;
    for (const auto& availableFormat : details.formats) {
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            this->format = availableFormat;
            formatFound = true;
        }
    }
    if (!formatFound) this->format = details.formats[0];

    if (details.capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
        this->extent = details.capabilities.currentExtent;
    } else {
        int width, height;
        glfwGetFramebufferSize(window->GetRawWindow(), &width, &height);

        VkExtent2D actualExtent = {
            static_cast<uint32_t>(width),
            static_cast<uint32_t>(height)
        };

        actualExtent.width = std::clamp(actualExtent.width, details.capabilities.minImageExtent.width, details.capabilities.maxImageExtent.width);
        actualExtent.height = std::clamp(actualExtent.height, details.capabilities.minImageExtent.height, details.capabilities.maxImageExtent.height);

        this->extent = actualExtent;
    }

    uint32_t imageCount = details.capabilities.minImageCount + 1;
    if (details.capabilities.maxImageCount > 0 && imageCount > details.capabilities.maxImageCount) {
        imageCount = details.capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR swapchainInfo{};
    swapchainInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchainInfo.surface = this->surface;
    swapchainInfo.minImageCount = imageCount;
    swapchainInfo.imageFormat = this->format.format;
    swapchainInfo.imageColorSpace = this->format.colorSpace;
    swapchainInfo.imageExtent = this->extent;
    swapchainInfo.imageArrayLayers = 1;
    swapchainInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    if (this->queueFamilies.graphics.value() != this->queueFamilies.present.value()) {
        uint32_t indicies[2] = {this->queueFamilies.graphics.value(), this->queueFamilies.present.value()};
        swapchainInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        swapchainInfo.queueFamilyIndexCount = 2;
        swapchainInfo.pQueueFamilyIndices = indicies;
    } else {
        swapchainInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        swapchainInfo.queueFamilyIndexCount = 0;
        swapchainInfo.pQueueFamilyIndices = nullptr;
    }

    swapchainInfo.preTransform = details.capabilities.currentTransform;
    swapchainInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapchainInfo.presentMode = VK_PRESENT_MODE_FIFO_KHR;
    swapchainInfo.clipped = VK_TRUE;
    swapchainInfo.oldSwapchain = nullptr;

    VkResult swapchainResult = vkCreateSwapchainKHR(this->device, &swapchainInfo, nullptr, &this->swapchain);
    if (swapchainResult != VK_SUCCESS) {
        CRITICAL("Swapchain creation failed with error code: {}", swapchainResult);
    }

    INFO("Vulkan swapchain created");

    vkGetSwapchainImagesKHR(this->device, this->swapchain, &imageCount, nullptr);
    this->images.resize(imageCount);
    vkGetSwapchainImagesKHR(this->device, this->swapchain, &imageCount, this->images.data());

    for (const auto& image : this->images) {
        VkImageViewCreateInfo imageViewInfo{};
        imageViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        imageViewInfo.image = image;
        imageViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        imageViewInfo.format = this->format.format;
        imageViewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        imageViewInfo.subresourceRange.baseMipLevel = 0;
        imageViewInfo.subresourceRange.levelCount = 1;
        imageViewInfo.subresourceRange.baseArrayLayer = 0;
        imageViewInfo.subresourceRange.layerCount = 1;

        VkImageView imageView;
        VkResult imageViewResult = vkCreateImageView(this->device, &imageViewInfo, nullptr, &imageView);
        if (imageViewResult != VK_SUCCESS) {
            CRITICAL("Image view creation failed with error code: {}", imageViewResult);
        }
        this->imageViews.push_back(imageView);
    }
}