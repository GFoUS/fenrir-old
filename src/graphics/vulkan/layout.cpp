#include "layout.h"

namespace tk
{
    VkPipelineLayout GetPipelineLayout(VkDevice device)
    {
        VkPipelineLayoutCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        createInfo.setLayoutCount = 0;
        createInfo.pSetLayouts = nullptr;
        createInfo.pushConstantRangeCount = 0;
        createInfo.pPushConstantRanges = nullptr;

        VkPipelineLayout layout;
        VkResult result = vkCreatePipelineLayout(device, &createInfo, nullptr, &layout);
        if (result != VK_SUCCESS)
        {
            CRITICAL("Pipeline layout creation failed with error code: {}", result);
        }
        return layout;
    }
}