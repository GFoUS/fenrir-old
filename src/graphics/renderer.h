#pragma once

#include "vulkan/vulkan.h"

class Renderer {
public:
    Renderer();
    ~Renderer();
    void CreateInstance();
private:
    VkInstance instance;
};