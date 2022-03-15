#pragma once

#include "core/core.h"
#include "vulkan/vulkan.h"
#include "physical.h"

namespace tk
{
    VkCommandPool CreateCommandPool(VkDevice device, VkPhysicalDevice physical);
    VkCommandBuffer CreateCommandBuffer(VkDevice device, VkCommandPool pool);
    void RecordCommandBuffer(VkCommandBuffer buffer, VkFramebuffer framebuffer, VkExtent2D extent, VkRenderPass renderPass, VkPipeline pipeline);
}