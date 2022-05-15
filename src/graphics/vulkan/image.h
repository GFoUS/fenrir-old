#pragma once

#include "core/core.h"
#include "vulkan/vulkan.h"
#include "vma.h"
#include "stb_image.h"
#include "buffer.h"
#include "context.h"
#include <vulkan/vulkan_core.h>

class Image {
public:
    Context* context{};

    VkImage image{};
    VmaAllocation allocation{};
    VkFormat format{};
    VkImageLayout currentLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    uint32_t width = 0;
    uint32_t height = 0;
    uint32_t mipLevels = 1;
    VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT;

    Image() = default;
    Image(Context* context, uint32_t width, uint32_t height, VkFormat format, VkImageUsageFlags usage, VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT, uint32_t mipLevels = 1) : context(context), format(format), width(width), height(height), mipLevels(mipLevels), samples(samples) {
        VkImageCreateInfo imageInfo = {};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.extent.width = this->width;
        imageInfo.extent.height = this->height;
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = this->mipLevels;
        imageInfo.arrayLayers = 1;
        imageInfo.format = this->format;
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage = usage;
        imageInfo.samples = samples;

        VmaAllocationCreateInfo allocInfo = {};
        allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

        VkResult result = vmaCreateImage(context->allocator, &imageInfo, &allocInfo, &this->image, &this->allocation, nullptr);
        if (result != VK_SUCCESS) {
            CRITICAL("Image creation failed with error code: {}", result);
        }
    }

    static std::unique_ptr<Image> LoadImage(Context* context, const std::string& filePath) {
        int width, height, channels;
        stbi_uc* pixels = stbi_load(filePath.c_str(), &width, &height, &channels, STBI_rgb_alpha);
        if (!pixels) {
            CRITICAL("Failed to load image: {}", filePath);
        }
        uint32_t mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(width, height)))) + 1;
        std::vector<uint8_t> pixelData(width * height * 4);
        memcpy(pixelData.data(), pixels, width * height * 4);
        stbi_image_free(pixels);

        Buffer<uint8_t> stagingBuffer(context, pixelData, VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
        std::unique_ptr<Image> image = std::make_unique<Image>(context, width, height, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, VK_SAMPLE_COUNT_1_BIT, mipLevels);
        context->StartAndSubmitCommandBuffer(context->graphics, [&image, &stagingBuffer](VkCommandBuffer commandBuffer) {
            image->TransitionLayout(commandBuffer, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
            image->CopyFrom(commandBuffer, stagingBuffer);
            image->GenerateMipmaps(commandBuffer);
        });

        return image;
    }

    void CopyFrom(VkCommandBuffer commandBuffer, Buffer<uint8_t> &src) {
        VkBufferImageCopy region{};
        region.bufferOffset = 0;
        region.bufferRowLength = 0;
        region.bufferImageHeight = 0;

        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.mipLevel = 0;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount = 1;

        region.imageOffset = {0, 0, 0};
        region.imageExtent = {
                this->width,
                this->height,
                1
        };

        vkCmdCopyBufferToImage(commandBuffer, src.buffer, this->image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
    }

    void TransitionLayout(VkCommandBuffer commandBuffer, VkImageLayout to) {
        VkImageMemoryBarrier barrier = {};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = this->currentLayout;
        barrier.newLayout = to;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = this->image;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = this->mipLevels;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;

        VkPipelineStageFlags sourceStage;
        VkPipelineStageFlags destinationStage;

        if (this->currentLayout == VK_IMAGE_LAYOUT_UNDEFINED && to == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
            barrier.srcAccessMask = 0;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

            sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        } else if (this->currentLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && to == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

            sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
            destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        } else {
            CRITICAL("Layout transition from {} to {} is not supported", this->currentLayout, to);
        }

        vkCmdPipelineBarrier(commandBuffer, sourceStage, destinationStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
        this->currentLayout = to;
    }

    void GenerateMipmaps(VkCommandBuffer commandBuffer) {
        VkFormatProperties formatProperties;
        vkGetPhysicalDeviceFormatProperties(this->context->physical, this->format, &formatProperties);

        if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)) {
            CRITICAL("Texture image format does not support linear blitting");
        }

        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.image = image;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;
        barrier.subresourceRange.levelCount = 1;

        int32_t mipWidth = this->width;
        int32_t mipHeight = this->height;

        for (uint32_t i = 1; i < mipLevels; i++) {
            barrier.subresourceRange.baseMipLevel = i - 1;
            barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

            vkCmdPipelineBarrier(commandBuffer,
                VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
                0, nullptr,
                0, nullptr,
                1, &barrier);

            VkImageBlit blit{};
            blit.srcOffsets[0] = { 0, 0, 0 };
            blit.srcOffsets[1] = { mipWidth, mipHeight, 1 };
            blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            blit.srcSubresource.mipLevel = i - 1;
            blit.srcSubresource.baseArrayLayer = 0;
            blit.srcSubresource.layerCount = 1;
            blit.dstOffsets[0] = { 0, 0, 0 };
            blit.dstOffsets[1] = { mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1 };
            blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            blit.dstSubresource.mipLevel = i;
            blit.dstSubresource.baseArrayLayer = 0;
            blit.dstSubresource.layerCount = 1;

            vkCmdBlitImage(commandBuffer,
                this->image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                this->image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                1, &blit,
                VK_FILTER_LINEAR);

            barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

            vkCmdPipelineBarrier(commandBuffer,
                VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
                0, nullptr,
                0, nullptr,
                1, &barrier);

            if (mipWidth > 1) mipWidth /= 2;
            if (mipHeight > 1) mipHeight /= 2;
        }

        barrier.subresourceRange.baseMipLevel = mipLevels - 1;
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        vkCmdPipelineBarrier(commandBuffer,
            VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
            0, nullptr,
            0, nullptr,
            1, &barrier);
    }

    VkImageView GetImageView(VkImageAspectFlagBits aspect) {
        if (this->imageView != nullptr) {
            return this->imageView;
        }

        VkImageViewCreateInfo viewInfo = {};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = this->image;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = this->format;
        viewInfo.subresourceRange.aspectMask = aspect;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = this->mipLevels;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;

        VkResult result = vkCreateImageView(context->device, &viewInfo, nullptr, &this->imageView);
        if (result != VK_SUCCESS) {
            CRITICAL("Image view creation failed with error code: {}", result);
        }

        return this->imageView;
    }

    VkSampler GetSampler(VkFilter magFilter = VK_FILTER_LINEAR, VkFilter minFilter = VK_FILTER_LINEAR, VkSamplerAddressMode wrapU = VK_SAMPLER_ADDRESS_MODE_REPEAT, VkSamplerAddressMode wrapV = VK_SAMPLER_ADDRESS_MODE_REPEAT) {
        if (this->sampler != nullptr) {
            return this->sampler;
        }

        VkSamplerCreateInfo samplerInfo{};
        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo.magFilter = magFilter;
        samplerInfo.minFilter = minFilter;
        samplerInfo.addressModeU = wrapU;
        samplerInfo.addressModeV = wrapV;
        samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.anisotropyEnable = VK_TRUE;
        samplerInfo.maxAnisotropy = context->physicalProperties.limits.maxSamplerAnisotropy;
        samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
        samplerInfo.unnormalizedCoordinates = VK_FALSE;
        samplerInfo.compareEnable = VK_FALSE;
        samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        samplerInfo.mipLodBias = 0.0f;
        samplerInfo.minLod = 0.0f;
        samplerInfo.maxLod = static_cast<float>(this->mipLevels);

        VkResult result = vkCreateSampler(context->device, &samplerInfo, nullptr, &this->sampler);
        if (result != VK_SUCCESS) {
            CRITICAL("Sampler creation failed with error code: {}", result);
        }

        return this->sampler;
    }

    void Destroy() {
        if (this->sampler != nullptr) {
            vkDestroySampler(context->device, this->sampler, nullptr);
        }
        if (this->imageView != nullptr) {
            vkDestroyImageView(context->device, this->imageView, nullptr);
        }
        vmaDestroyImage(context->allocator, this->image, this->allocation);
        this->isDestroyed = true;
    }

    ~Image() {
        if (!this->isDestroyed) this->Destroy();
    }
private:
    VkImageView imageView = nullptr;
    VkSampler sampler = nullptr;
    bool isDestroyed = false;
};