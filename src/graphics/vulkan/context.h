#include "vulkan/vulkan.h"
#include "../window.h"
#include "core/core.h"
#include "utils.h"
#include <vulkan/vulkan_core.h>

struct Context {
    Context(Window* window);
    ~Context();

    void CreateDevice();
    void CreateSwapchain();
    void CreatePipeline();

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
};