#include "surface.h"

namespace tk {
    VkSurfaceKHR GetSurface(VkInstance instance, Window *window) {
        VkSurfaceKHR surface;
        GLFWwindow* rawWindow = window->GetRawWindow();
        VkResult result = glfwCreateWindowSurface(instance, rawWindow, nullptr, &surface);
        if (result != VK_SUCCESS) {
            CRITICAL("Vulkan surface creation failed with error code: {}", result);
        }
        return surface;
    }
}