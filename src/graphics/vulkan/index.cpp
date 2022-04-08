#include "index.h"

#include "context.h"
#include "utils.h"

IndexBuffer::IndexBuffer(Context* context, std::vector<uint32_t> indices) : context(context), indices(indices) {
    VkDeviceSize size = sizeof(Vertex) * this->indices.size();

    VkBuffer staging;
    VkDeviceMemory stagingMemory;

    CreateBuffer(context, staging, stagingMemory, size, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, VK_BUFFER_USAGE_TRANSFER_SRC_BIT);

    void* data;
    vkMapMemory(context->device, stagingMemory, 0, size, 0, &data);
    memcpy(data, this->indices.data(), size);
    vkUnmapMemory(context->device, stagingMemory);

    CreateBuffer(context, this->buffer, this->memory, size, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
    CopyBuffer(context, staging, this->buffer, size);

    vkDestroyBuffer(context->device, staging, nullptr);
    vkFreeMemory(context->device, stagingMemory, nullptr);
}

void IndexBuffer::Destroy() {
    vkDestroyBuffer(context->device, this->buffer, nullptr);
    vkFreeMemory(context->device, this->memory, nullptr);
}