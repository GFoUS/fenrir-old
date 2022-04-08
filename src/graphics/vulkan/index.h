#pragma once

#include "glm/glm.hpp"
#include "vulkan/vulkan.h"
#include "core/core.h"

struct Context;

struct IndexBuffer {
    std::vector<uint32_t> indices;
    VkBuffer buffer;
    VkDeviceMemory memory;
    Context* context;

    IndexBuffer() = default;
    IndexBuffer(Context* context, std::vector<uint32_t> indices);
    void Destroy();
};