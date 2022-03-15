#include "command.h"

namespace tk
{
    VkCommandPool CreateCommandPool(VkDevice device, VkPhysicalDevice physical)
    {
        VkCommandPoolCreateInfo createInfo{};
        createInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        createInfo.pNext = nullptr;
        createInfo.queueFamilyIndex = GetGraphicsQueueFamily(physical).index;
        createInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;

        VkCommandPool pool;
        VkResult result = vkCreateCommandPool(device, &createInfo, nullptr, &pool);
        if (result != VK_SUCCESS)
        {
            CRITICAL("Command pool creation failed with error code: {}", result);
        }
        return pool;
    }

    VkCommandBuffer CreateCommandBuffer(VkDevice device, VkCommandPool pool)
    {
        VkCommandBuffer buffer;

        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.commandBufferCount = 1;
        allocInfo.commandPool = pool;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.pNext = nullptr;
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;

        VkResult result = vkAllocateCommandBuffers(device, &allocInfo, &buffer);
        if (result != VK_SUCCESS)
        {
            CRITICAL("Command buffer creation failed with error code: {}", result);
        }

        return buffer;
    }

    void RecordCommandBuffer(VkCommandBuffer buffer, VkFramebuffer framebuffer, VkExtent2D extent, VkRenderPass renderPass, VkPipeline pipeline)
    {
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = 0;
        beginInfo.pInheritanceInfo = nullptr;
        beginInfo.pNext = nullptr;

        VkResult beginResult = vkBeginCommandBuffer(buffer, &beginInfo);
        if (beginResult != VK_SUCCESS)
        {
            CRITICAL("Begin recording command buffer failed with error code: {}", beginResult);
        }

        VkRenderPassBeginInfo renderPassInfo{};
        VkClearValue clearColour = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
        ;
        renderPassInfo.clearValueCount = 1;
        renderPassInfo.framebuffer = framebuffer;
        renderPassInfo.pClearValues = &clearColour;
        renderPassInfo.pNext = nullptr;
        renderPassInfo.renderArea.extent = extent;
        renderPassInfo.renderArea.offset = {0, 0};
        renderPassInfo.renderPass = renderPass;
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        vkCmdBeginRenderPass(buffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

        vkCmdBindPipeline(buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

        vkCmdDraw(buffer, 3, 1, 0, 0);

        vkCmdEndRenderPass(buffer);

        VkResult endResult = vkEndCommandBuffer(buffer);
        if (endResult != VK_SUCCESS)
        {
            CRITICAL("Command buffer recording failed with error code: {}", endResult);
        }
    }
}