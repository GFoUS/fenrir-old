#pragma once

#include "vulkan/vulkan.h"
#include "../window.h"
#include "core/core.h"
#include "utils.h"
#include "shader.h"
#include "vma.h"
#include "image.h"

struct Context {
    Context(Window* window);
    ~Context();

    void CreateDevice();
    void CreateSwapchain();
    void CreatePipeline();
    void CreateUniformBuffer();
    void CreateDepthBuffer();

    void StartCommandBuffer(VkCommandBuffer buffer, uint32_t imageIndex);
    void EndCommandBuffer(VkCommandBuffer buffer);

    VkInstance instance;
    VkPhysicalDevice physical;
    VkPhysicalDeviceProperties physicalProperties;
    QueueFamilies queueFamilies;
    VkDevice device;
    VkQueue graphics;
    VkQueue present;
    VmaAllocator allocator;
    
    Window* window;
    VkSurfaceKHR surface;
    VkSurfaceFormatKHR format;
    VkExtent2D extent;
    VkSwapchainKHR swapchain;
    std::vector<VkImage> images;
    std::vector<VkImageView> imageViews;
    std::vector<VkFramebuffer> framebuffers;

    VkPipelineLayout pipelineLayout;
    VkRenderPass renderPass;
    VkPipeline pipeline;

    VkCommandPool commandPool;
    VkCommandBuffer commandBuffer;

    VkSemaphore imageAvailableSemaphore;
    VkSemaphore renderFinishedSemaphore;
    VkFence inFlightFence;

    std::vector<VkDescriptorSetLayout> descriptorSetLayouts;
    VkBuffer uniformBuffer;
    VkDeviceMemory uniformBufferMemory;
    VkDescriptorPool descriptorPool;
    VkDescriptorSet descriptorSet;

    std::unique_ptr<Image> depthImage;

    VkSampleCountFlagBits msaaSamples{};
    std::unique_ptr<Image> colorImage;
};