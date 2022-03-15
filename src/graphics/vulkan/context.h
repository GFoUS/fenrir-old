#include "core/core.h"
#include "graphics/window.h"
#include "vulkan/vulkan.h"
#include "swapchain.h"

struct Context {
    Context(Window* window);
    ~Context();

    Window* window;
    VkInstance instance;
    VkPhysicalDevice physical;
    VkDevice device;
    VkQueue graphicsQueue;
    VkQueue presentQueue;
    VkSurfaceKHR surface;
    tk::Swapchain swapchain;
    std::vector<VkImage> images;
    std::vector<VkImageView> imageViews;
    VkRenderPass renderPass;
    VkPipelineLayout layout;
    VkPipeline pipeline;
    std::vector<VkFramebuffer> framebuffers;
    VkCommandPool commandPool;
    VkCommandBuffer buffer;

    VkSemaphore imageAvailable;
    VkSemaphore renderFinished;
    VkFence inFlight;

    void DrawFrame();
private:
    void CreateSyncObjects();
};