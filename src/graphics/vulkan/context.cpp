#include "context.h"
#include "utils.h"
#define GLFW_INCLUDE_VULKAN
#include "GLFW/glfw3.h"
#include "uniform.h"
#include "vertex.h"
#include "image.h"
#include "pipeline.h"

const std::vector<const char*> instanceExtensions = {};
const std::vector<const char*> validationLayers = {"VK_LAYER_KHRONOS_validation"};
const std::vector<const char*> deviceExtensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

Context::Context(Window* window) : window(window) {
    INFO("Initializing Vulkan");

    this->CreateDevice();
    this->CreateSwapchain();
    this->CreatePipeline();
    this->CreateUniformBuffer();

    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    if (vkCreateSemaphore(this->device, &semaphoreInfo, nullptr, &this->imageAvailableSemaphore) != VK_SUCCESS ||
        vkCreateSemaphore(this->device, &semaphoreInfo, nullptr, &this->renderFinishedSemaphore) != VK_SUCCESS ||
        vkCreateFence(this->device, &fenceInfo, nullptr, &this->inFlightFence) != VK_SUCCESS){

        CRITICAL("failed to create synchronization objects");
    }
}

Context::~Context() {
    vkDeviceWaitIdle(this->device);

    vkDestroyDescriptorPool(this->device, this->descriptorPool, nullptr);
    for (auto& descriptorSetLayout : this->descriptorSetLayouts) {
        vkDestroyDescriptorSetLayout(this->device, descriptorSetLayout, nullptr);
    }
    vkDestroyBuffer(this->device, this->uniformBuffer, nullptr);
    vkFreeMemory(this->device, this->uniformBufferMemory, nullptr);
    vkDestroySemaphore(this->device, this->imageAvailableSemaphore, nullptr);
    vkDestroySemaphore(this->device, this->renderFinishedSemaphore, nullptr);
    vkDestroyFence(this->device, this->inFlightFence, nullptr);
    vkDestroyCommandPool(this->device, this->commandPool, nullptr);
    for (auto& framebuffer : this->framebuffers) {
        vkDestroyFramebuffer(this->device, framebuffer, nullptr);
    }   
    this->colorImage->Destroy();
    this->depthImage->Destroy();
    this->pipeline->Destroy();
    for (auto& imageView : this->imageViews) {
        vkDestroyImageView(this->device, imageView, nullptr);
    }
    vkDestroySwapchainKHR(this->device, this->swapchain, nullptr);
    vmaDestroyAllocator(this->allocator);
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
    vkGetPhysicalDeviceProperties(this->physical, &this->physicalProperties);
    this->queueFamilies = FindQueueFamilies(this);
    this->msaaSamples = GetMaxUsableSampleCount(this->physicalProperties);

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
    deviceFeatures.samplerAnisotropy = VK_TRUE;

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

    VmaVulkanFunctions vulkanFunctions = {};
    vulkanFunctions.vkGetInstanceProcAddr = &vkGetInstanceProcAddr;
    vulkanFunctions.vkGetDeviceProcAddr = &vkGetDeviceProcAddr;

    VmaAllocatorCreateInfo allocatorCreateInfo{};
    allocatorCreateInfo.vulkanApiVersion = VK_API_VERSION_1_0;
    allocatorCreateInfo.physicalDevice = this->physical;
    allocatorCreateInfo.device = this->device;
    allocatorCreateInfo.instance = this->instance;
    allocatorCreateInfo.pVulkanFunctions = &vulkanFunctions;
    allocatorCreateInfo.flags = 0;
    vmaCreateAllocator(&allocatorCreateInfo, &this->allocator);
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

void Context::CreatePipeline() {
    Shader vertex(this->device, "shaders/vert.spv", VERTEX);
    Shader fragment(this->device, "shaders/frag.spv", FRAGMENT);

    PipelineBuilder builder(this);
    this->pipeline = builder
        .SetDepthTesting(true)
        .SetMsaaSamples(this->msaaSamples)
        .SetShader(&vertex)
        .SetShader(&fragment)
        .SetViewport(this->extent.width, this->extent.height)
        .Build();
    
    this->colorImage = std::make_unique<Image>(this, this->extent.width, this->extent.height, this->format.format, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, this->msaaSamples);
    this->depthImage = std::make_unique<Image>(this, this->extent.width, this->extent.height, VK_FORMAT_D32_SFLOAT, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, this->msaaSamples);

    this->framebuffers.resize(this->imageViews.size());
    for (uint32_t i = 0; i < this->imageViews.size(); i++) {
        std::vector<VkImageView> attachments = { this->colorImage->GetImageView(VK_IMAGE_ASPECT_COLOR_BIT), this->depthImage->GetImageView(VK_IMAGE_ASPECT_DEPTH_BIT), this->imageViews[i] };

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.width = this->extent.width;
        framebufferInfo.height = this->extent.height;
        framebufferInfo.renderPass = this->pipeline->renderPass;
        framebufferInfo.attachmentCount = attachments.size();
        framebufferInfo.pAttachments = attachments.data();
        framebufferInfo.layers = 1;

        VkResult framebufferResult = vkCreateFramebuffer(this->device, &framebufferInfo, nullptr, &this->framebuffers[i]);
        if (framebufferResult != VK_SUCCESS) {
            CRITICAL("Framebuffer creation failed with error code: {}", framebufferResult);
        }
    }

    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = this->queueFamilies.graphics.value();

    VkResult poolResult = vkCreateCommandPool(this->device, &poolInfo, nullptr, &this->commandPool);
    if (poolResult != VK_SUCCESS) {
        CRITICAL("Command pool creation failed with error code: {}", poolResult);
    }

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = 1;
    
    VkResult allocResult = vkAllocateCommandBuffers(this->device, &allocInfo, &this->commandBuffer);
    if (allocResult != VK_SUCCESS) {
        CRITICAL("Command buffer allocation failed with error code: {}", allocResult);
    }
}

void Context::StartCommandBuffer(VkCommandBuffer buffer, uint32_t imageIndex) {
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    VkResult beginResult = vkBeginCommandBuffer(buffer, &beginInfo);
    if (beginResult != VK_SUCCESS) {
        CRITICAL("Failed to begin command buffer with error code: {}", beginResult);
    }

    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = this->pipeline->renderPass;
    renderPassInfo.framebuffer = this->framebuffers[imageIndex];
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = this->extent;
    std::array<VkClearValue, 2> clearValues{};
    clearValues[0].color = {{0.0f, 0.0f, 0.0f, 1.0f}};
    clearValues[1].depthStencil = {1.0f, 0};
    renderPassInfo.clearValueCount = clearValues.size();
    renderPassInfo.pClearValues = clearValues.data();
    vkCmdBeginRenderPass(buffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, this->pipeline->pipeline);
    //vkCmdBindDescriptorSets(buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, this->pipelineLayout, 1, 1, &this->descriptorSet, 0, nullptr);
}

void Context::EndCommandBuffer(VkCommandBuffer buffer) {
    vkCmdEndRenderPass(buffer);
    VkResult endResult = vkEndCommandBuffer(buffer);
    if (endResult != VK_SUCCESS) {
        CRITICAL("Failed to end command buffer with error code: {}", endResult);
    }
}

void Context::CreateUniformBuffer() {
    VkDeviceSize size = sizeof(UniformBufferObject);
    CreateBuffer(this, this->uniformBuffer, this->uniformBufferMemory, size, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);

    VkDescriptorPoolSize poolSizes[] =
    {
        { VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
        { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
        { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
    }; // Make descriptor allocator better so this doesn't overallocate

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = std::size(poolSizes);
    poolInfo.pPoolSizes = poolSizes;
    poolInfo.maxSets = 1000;

    VkResult poolResult = vkCreateDescriptorPool(this->device, &poolInfo, nullptr, &this->descriptorPool);
    if (poolResult != VK_SUCCESS) {
        CRITICAL("Descriptor pool creation failed with error code: {}", poolResult);
    }

    /*
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = this->descriptorPool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &this->descriptorSetLayouts[1]; // Set to second set which is the view and projection matrices, model matrix is handled in the model class

    VkResult allocResult = vkAllocateDescriptorSets(this->device, &allocInfo, &this->descriptorSet);
    if (allocResult != VK_SUCCESS) {
        CRITICAL("Descriptor set allocation failed with error code: {}", allocResult);
    }

    VkDescriptorBufferInfo bufferInfo{};
    bufferInfo.buffer = this->uniformBuffer;
    bufferInfo.offset = 0;
    bufferInfo.range = size;

    VkWriteDescriptorSet descriptorWrite{};
    descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite.dstSet = this->descriptorSet;
    descriptorWrite.dstBinding = 0;
    descriptorWrite.dstArrayElement = 0;
    descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptorWrite.descriptorCount = 1;
    descriptorWrite.pBufferInfo = &bufferInfo;

    vkUpdateDescriptorSets(this->device, 1, &descriptorWrite, 0, nullptr);
    */
}

void Context::StartAndSubmitCommandBuffer(VkQueue queue, const std::function<void(VkCommandBuffer)>& body) const {
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = this->commandPool;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer cmdBuffer;
    VkResult allocResult = vkAllocateCommandBuffers(this->device, &allocInfo, &cmdBuffer);
    if (allocResult != VK_SUCCESS) {
        CRITICAL("Command buffer allocation during copy failed with error code: {}", allocResult);
    }

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    VkResult beginResult = vkBeginCommandBuffer(cmdBuffer, &beginInfo);
    if (beginResult != VK_SUCCESS) {
        CRITICAL("Beginning copy command buffer failed with error code: {}", beginResult);
    }

    body(cmdBuffer);

    VkResult endResult = vkEndCommandBuffer(cmdBuffer);
    if (endResult != VK_SUCCESS) {
        CRITICAL("Ending copy command buffer failed with error code: {}", endResult);
    }

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &cmdBuffer;

    VkResult submitResult = vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
    if (submitResult != VK_SUCCESS) {
        CRITICAL("Submitting copy command buffer failed with error code: {}", submitResult);
    }
    vkQueueWaitIdle(queue);

    vkFreeCommandBuffers(this->device, this->commandPool, 1, &cmdBuffer);
}

