#include "context.h"
#include "utils.h"
#define GLFW_INCLUDE_VULKAN
#include "GLFW/glfw3.h"
#include "uniform.h"
#include "vertex.h"
#include "image.h"
#include "descriptor.h"

const std::vector<const char*> instanceExtensions = {};
const std::vector<const char*> validationLayers = {"VK_LAYER_KHRONOS_validation"};
const std::vector<const char*> deviceExtensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

Context::Context(Window* window) : window(window) {
    INFO("Initializing Vulkan");

    this->CreateDevice();
    this->CreateSwapchain();
    this->depthImage = std::make_unique<Image>(this, this->extent.width, this->extent.height, VK_FORMAT_D32_SFLOAT, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, this->msaaSamples);
    this->colorImage = std::make_unique<Image>(this, this->extent.width, this->extent.height, this->format.format, VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, this->msaaSamples);
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
    this->depthImage->Destroy();
    this->colorImage->Destroy();
    vkDestroyPipeline(this->device, this->pipeline, nullptr);
    vkDestroyRenderPass(this->device, this->renderPass, nullptr);
    vkDestroyPipelineLayout(this->device, this->pipelineLayout, nullptr);
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
    VkPipelineShaderStageCreateInfo shaderStages[2] = {vertex.GetStageInfo(), fragment.GetStageInfo()};

    auto bindingDescription = Vertex::GetBindingDescription();
    auto attributeDescriptions = Vertex::GetAttributeDescriptions();
    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
    vertexInputInfo.vertexAttributeDescriptionCount = attributeDescriptions.size();
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float) this->extent.width;
    viewport.height = (float) this->extent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = extent;

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports = &viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &scissor;

    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;

    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = this->msaaSamples;

    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;

    VkPipelineDepthStencilStateCreateInfo depthStencil{};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = VK_TRUE;
    depthStencil.depthWriteEnable = VK_TRUE;
    depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.stencilTestEnable = VK_FALSE;

    VkDescriptorSetLayoutBinding modelLayoutBinding{};
    modelLayoutBinding.binding = 0;
    modelLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    modelLayoutBinding.descriptorCount = 1;
    modelLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    VkDescriptorSetLayoutCreateInfo modelLayout{};
    modelLayout.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    modelLayout.bindingCount = 1;
    modelLayout.pBindings = &modelLayoutBinding;

    VkDescriptorSetLayoutBinding viewProjectionLayoutBinding{};
    viewProjectionLayoutBinding.binding = 0;
    viewProjectionLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    viewProjectionLayoutBinding.descriptorCount = 1;
    viewProjectionLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    VkDescriptorSetLayoutCreateInfo viewProjectionLayout{};
    viewProjectionLayout.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    viewProjectionLayout.bindingCount = 1;
    viewProjectionLayout.pBindings = &viewProjectionLayoutBinding;

    VkDescriptorSetLayoutBinding textureBinding{};
    textureBinding.binding = 0;
    textureBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    textureBinding.descriptorCount = 1;
    textureBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    VkDescriptorSetLayoutCreateInfo textureLayout{};
    textureLayout.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    textureLayout.bindingCount = 1;
    textureLayout.pBindings = &textureBinding;


    std::vector<VkDescriptorSetLayoutCreateInfo> layoutInfos = {modelLayout, viewProjectionLayout, textureLayout};
    this->descriptorSetLayouts.resize(layoutInfos.size());

    for (uint32_t i = 0; i < layoutInfos.size(); i++) {
        VkResult descriptorSetLayoutResult = vkCreateDescriptorSetLayout(this->device, &layoutInfos[i], nullptr, &this->descriptorSetLayouts[i]);
        if (descriptorSetLayoutResult != VK_SUCCESS) {
            CRITICAL("Descriptor set layout creation failed with error code: {}", descriptorSetLayoutResult);
        }
    }

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = this->descriptorSetLayouts.size();
    pipelineLayoutInfo.pSetLayouts = this->descriptorSetLayouts.data();

    VkResult layoutResult = vkCreatePipelineLayout(this->device, &pipelineLayoutInfo, nullptr, &this->pipelineLayout);
    if (layoutResult != VK_SUCCESS) {
        CRITICAL("Vulkan pipeline layout creation failed with error code: {}", layoutResult);
    }

    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = this->format.format;
    colorAttachment.samples = this->msaaSamples;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;  
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentDescription depthAttachment{};
    depthAttachment.format = this->depthImage->format;
    depthAttachment.samples = this->msaaSamples;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depthAttachmentRef{};
    depthAttachmentRef.attachment = 1;
    depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentDescription colorAttachmentResolve{};
    colorAttachmentResolve.format = this->format.format;
    colorAttachmentResolve.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachmentResolve.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachmentResolve.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachmentResolve.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachmentResolve.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachmentResolve.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachmentResolve.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorAttachmentResolveRef{};
    colorAttachmentResolveRef.attachment = 2;
    colorAttachmentResolveRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;
    subpass.pDepthStencilAttachment = &depthAttachmentRef;
    subpass.pResolveAttachments = &colorAttachmentResolveRef;

    std::array<VkAttachmentDescription, 3> attachments = {colorAttachment, depthAttachment, colorAttachmentResolve};
    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = attachments.size();
    renderPassInfo.pAttachments = attachments.data();
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;

    VkResult renderPassResult = vkCreateRenderPass(this->device, &renderPassInfo, nullptr, &this->renderPass);
    if (renderPassResult != VK_SUCCESS) {
        CRITICAL("Vulkan render pass creation failed with error code: {}", renderPassResult);
    }
    INFO("Created render pass");

    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDepthStencilState = &depthStencil;
    pipelineInfo.layout = this->pipelineLayout;
    pipelineInfo.renderPass = this->renderPass;
    pipelineInfo.subpass = 0;

    VkResult pipelineResult = vkCreateGraphicsPipelines(this->device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &this->pipeline);
    if (pipelineResult != VK_SUCCESS) {
        CRITICAL("Vulkan pipeline creation failed with error code: {}", pipelineResult);
    }
    INFO("Vulkan pipeline created");

    vertex.Destroy();
    fragment.Destroy();

    for (const auto& imageView : this->imageViews) {
        std::array<VkImageView, 3> framebufferAttachments = {this->colorImage->GetImageView(VK_IMAGE_ASPECT_COLOR_BIT), this->depthImage->GetImageView(VK_IMAGE_ASPECT_DEPTH_BIT), imageView};
        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = renderPass;
        framebufferInfo.attachmentCount = framebufferAttachments.size();
        framebufferInfo.pAttachments = framebufferAttachments.data();
        framebufferInfo.width = this->extent.width;
        framebufferInfo.height = this->extent.height;
        framebufferInfo.layers = 1;

        VkFramebuffer framebuffer;
        VkResult framebufferResult = vkCreateFramebuffer(this->device, &framebufferInfo, nullptr, &framebuffer);
        if (framebufferResult != VK_SUCCESS) {
            CRITICAL("Framebuffer creation failed with error code: {}", framebufferResult);
        }

        this->framebuffers.push_back(framebuffer);
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
    renderPassInfo.renderPass = renderPass;
    renderPassInfo.framebuffer = this->framebuffers[imageIndex];
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = this->extent;
    std::array<VkClearValue, 2> clearValues{};
    clearValues[0].color = {{0.0f, 0.0f, 0.0f, 1.0f}};
    clearValues[1].depthStencil = {1.0f, 0};
    renderPassInfo.clearValueCount = clearValues.size();
    renderPassInfo.pClearValues = clearValues.data();
    vkCmdBeginRenderPass(buffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, this->pipeline);
    vkCmdBindDescriptorSets(buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, this->pipelineLayout, 1, 1, &this->descriptorSet, 0, nullptr);
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

    VkDescriptorPoolSize poolSize{};
    poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSize.descriptorCount = 1;

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = 1;
    poolInfo.pPoolSizes = &poolSize;
    poolInfo.maxSets = 1;

    VkResult poolResult = vkCreateDescriptorPool(this->device, &poolInfo, nullptr, &this->descriptorPool);
    if (poolResult != VK_SUCCESS) {
        CRITICAL("Descriptor pool creation failed with error code: {}", poolResult);
    }

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
