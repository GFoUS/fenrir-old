#include "vulkan/vulkan.h"
#include "../window.h"
#include "core/core.h"
#include "utils.h"
#include "shader.h"
#include "vertex.h"
#include <vulkan/vulkan_core.h>

struct Context {
    Context(Window* window, std::vector<Vertex> vertices, std::vector<uint32_t> indices);
    ~Context();

    void CreateDevice();
    void CreateSwapchain();
    void CreatePipeline();
    void CreateVertexBuffer();
    void CreateIndexBuffer();
    void CreateUniformBuffer();
    void RecordCommandBuffer(VkCommandBuffer buffer, uint32_t imageIndex);

    VkInstance instance;
    VkPhysicalDevice physical;
    QueueFamilies queueFamilies;
    VkDevice device;
    VkQueue graphics;
    VkQueue present;
    
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

    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    VkBuffer vertexBuffer;
    VkBuffer indexBuffer;
    VkDeviceMemory vertexBufferMemory;
    VkDeviceMemory indexBufferMemory;

    VkDescriptorSetLayout descriptorSetLayout;
    VkBuffer uniformBuffer;
    VkDeviceMemory uniformBufferMemory;
    VkDescriptorPool descriptorPool;
    VkDescriptorSet descriptorSet;
};