#pragma once

#include "core/core.h"
#include "vulkan/vulkan.h"
#include "vma.h"
#include "context.h"

template <class T> class Buffer {
public:
    Buffer<T>() = default;
    Buffer<T>(Context* context, std::vector<T> data, VkBufferUsageFlags usage) : context(context) {
        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = data.size() * sizeof(T);
        bufferInfo.usage = usage;

        VmaAllocationCreateInfo allocInfo{};
        allocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;

        VkResult allocResult = vmaCreateBuffer(context->allocator, &bufferInfo, &allocInfo, &this->buffer, &this->allocation, nullptr);
        if (allocResult != VK_SUCCESS) {
            CRITICAL("Buffer allocation failed with error code: {}", allocResult)
        }

        void* mapping;
        vmaMapMemory(context->allocator, this->allocation, &mapping);
        memcpy(mapping, data.data(), data.size() * sizeof(T));
        vmaUnmapMemory(context->allocator, this->allocation);
    }

    ~Buffer<T>() {
        if (!this->isDestroyed) this->Destroy();
    }

    void CopyTo(Buffer<T> &dest, VkCommandBuffer commandBuffer) {
        VkBufferCopy copy{};
        copy.srcOffset = 0;
        copy.dstOffset = 0;
        copy.size = this->size;
        vkCmdCopyBuffer(commandBuffer, this->buffer, dest.buffer, 1, &copy);
    }

    void Update(std::vector<T> data) {
        void* mapping;
        vmaMapMemory(context->allocator, this->allocation, &mapping);
        memcpy(mapping, data.data(), data.size() * sizeof(T));
        vmaUnmapMemory(context->allocator, this->allocation);
    }

    void Destroy() {
        vmaDestroyBuffer(context->allocator, this->buffer, this->allocation);
        this->isDestroyed = true;
    }

    struct Ref {
        Buffer<T> *buffer;
        uint64_t offset;
        uint64_t size;
    };

    Ref GetRef(uint64_t offset, uint64_t size) {
        return {
            this,
            offset,
            size
        };
    }

    VkBuffer buffer{};
    VmaAllocation allocation{};
    uint32_t size{};

    Context* context;
private:
    bool isDestroyed = false;
};


