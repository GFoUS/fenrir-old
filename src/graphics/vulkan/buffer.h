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

    VkBuffer buffer{};
    VmaAllocation allocation{};
    uint32_t size{};

private:
    Context* context;
    bool isDestroyed = false;
};


