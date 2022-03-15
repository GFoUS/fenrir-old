#include "framebuffer.h"

namespace tk {
    std::vector<VkFramebuffer> GetFramebuffers(VkDevice device, std::vector<VkImageView> imageViews, VkRenderPass renderPass, VkExtent2D extent) {
        std::vector<VkFramebuffer>  framebuffers(imageViews.size());

        for (uint32_t i = 0; i < imageViews.size(); i++) {
            VkFramebufferCreateInfo createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            createInfo.renderPass = renderPass;
            createInfo.attachmentCount = 1;
            createInfo.pAttachments = &imageViews[i];
            createInfo.width = extent.width;
            createInfo.height = extent.height;
            createInfo.layers = 1;

            VkResult result = vkCreateFramebuffer(device, &createInfo, nullptr, &framebuffers[i]);
            if (result != VK_SUCCESS) {
                CRITICAL("Framebuffer creation failed with error code: {}", result);
            }
        }

        return framebuffers;
    }
}