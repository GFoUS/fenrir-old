#pragma once

#define GLFW_INCLUDE_VULKAN
#include "GLFW/glfw3.h"
#include "graphics/window.h"

namespace tk
{
    VkSurfaceKHR GetSurface(VkInstance instance, Window *window);
}