#include "core/core.h"
#include "graphics/window.h"
#include "vulkan/vulkan.h"

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
};