#include "vulkan/vulkan.h"
#include "../window.h"
#include "core/core.h"
#include "utils.h"
#include "shader.h"
#include "vertex.h"

struct Context {
    Context(Window* window);
    ~Context();

    void CreateDevice();
    void CreateSwapchain();
    void CreatePipeline();
    void RecordCommandBuffer(VkCommandBuffer buffer, uint32_t imageIndex);
    void DrawFrame();

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

    VkPipelineLayout layout;
    VkRenderPass renderPass;
    VkPipeline pipeline;

    VkCommandPool commandPool;
    VkCommandBuffer commandBuffer;

    VkSemaphore imageAvailableSemaphore;
    VkSemaphore renderFinishedSemaphore;
    VkFence inFlightFence;
};