#pragma once

#include "core/core.h"
#include "vulkan/vulkan.h"
#include "vma.h"

class Image {
public:
    VmaAllocator allocator;
    VkDevice device;

    VkImage image;
    VmaAllocation allocation;
    VkFormat format;

    Image() = default;
    Image(VmaAllocator allocator, uint32_t width, uint32_t height, VkFormat format, VkImageUsageFlags usage, VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT) : allocator(allocator), format(format) {
        VkImageCreateInfo imageInfo = {};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.extent.width = width;
        imageInfo.extent.height = height;
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = 1;
        imageInfo.format = format;
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage = usage;
        imageInfo.samples = samples;

        VmaAllocationCreateInfo allocInfo = {};
        allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

        VkResult result = vmaCreateImage(this->allocator, &imageInfo, &allocInfo, &this->image, &this->allocation, nullptr);
        if (result != VK_SUCCESS) {
            CRITICAL("Image creation failed with error code: {}", result);
        }
    }

    VkImageView GetImageView(VkDevice d, VkImageAspectFlagBits aspect) {
        if (this->imageView != nullptr) {
            return this->imageView;
        }

        this->device = d;

        VkImageViewCreateInfo viewInfo = {};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = this->image;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = this->format;
        viewInfo.subresourceRange.aspectMask = aspect;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;

        VkResult result = vkCreateImageView(this->device, &viewInfo, nullptr, &this->imageView);
        if (result != VK_SUCCESS) {
            CRITICAL("Image view creation failed with error code: {}", result);
        }

        return this->imageView;
    }

    void Destroy() {
        if (this->imageView != nullptr) {
            vkDestroyImageView(this->device, this->imageView, nullptr);
        }
        vmaDestroyImage(this->allocator, this->image, this->allocation);
        this->isDestroyed = true;
    }

    ~Image() {
        if (!this->isDestroyed) this->Destroy();
    }
private:
    VkImageView imageView;
    bool isDestroyed = false;
};