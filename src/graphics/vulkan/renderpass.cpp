#include "renderpass.h"

namespace tk
{
    VkRenderPass GetRenderPass(VkDevice device, Swapchain swapchain)
    {
        VkAttachmentDescription colourAttachment{};
        colourAttachment.format = swapchain.format.format;
        colourAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        colourAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colourAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colourAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colourAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colourAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        colourAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        VkAttachmentReference colourAttachmentRef{};
        colourAttachmentRef.attachment = 0;
        colourAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &colourAttachmentRef;

        VkSubpassDependency dependency{};
        dependency.dependencyFlags = 0;
        dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.dstSubpass = 0;
        dependency.srcAccessMask = 0;
        dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.srcSubpass = VK_SUBPASS_EXTERNAL;

        VkRenderPassCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        createInfo.attachmentCount = 1;
        createInfo.pAttachments = &colourAttachment;
        createInfo.subpassCount = 1;
        createInfo.pSubpasses = &subpass;
        createInfo.dependencyCount = 1;
        createInfo.pDependencies = &dependency;

        VkRenderPass renderPass;
        VkResult result = vkCreateRenderPass(device, &createInfo, nullptr, &renderPass);
        if (result != VK_SUCCESS) {
            CRITICAL("Render pass creation failed with error code: {}", result);
        }

        return renderPass;
    }
}