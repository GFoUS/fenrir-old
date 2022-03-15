#pragma once

#include "core/core.h"
#include "vulkan/vulkan.h"
#include "swapchain.h"

namespace tk {
    VkPipeline GetPipeline(VkDevice device, Swapchain swapchain, std::vector<VkPipelineShaderStageCreateInfo> shaders, VkPipelineLayout layout, VkRenderPass renderPass);

    VkPipelineVertexInputStateCreateInfo _GetVertexInputInfo();
    VkPipelineInputAssemblyStateCreateInfo _GetInputAssemblyInfo();
    VkPipelineViewportStateCreateInfo _GetViewportInfo(VkExtent2D extent);
    VkPipelineRasterizationStateCreateInfo _GetRasterizationInfo();
    VkPipelineMultisampleStateCreateInfo _GetMultisamplingInfo();
    VkPipelineColorBlendStateCreateInfo _GetBlendStateInfo();
}