#include "renderer.h"

#define GLM_FORCE_RADIANS
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "chrono"
#include "core/core.h"
#include "vulkan/uniform.h"

const std::vector<Vertex> vertices = {
    {{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},
    {{0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
    {{0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}},
    {{-0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}}
};

const std::vector<uint32_t> indices = {
    0, 1, 2, 2, 3, 0
};

Renderer::Renderer(Window *window) : context(window, vertices, indices) {}

Renderer::~Renderer() {}

void Renderer::OnTick() {
    vkWaitForFences(context.device, 1, &context.inFlightFence, VK_TRUE, UINT64_MAX);
    vkResetFences(context.device, 1, &context.inFlightFence);

    uint32_t imageIndex;
    VkResult acquireResult = vkAcquireNextImageKHR(context.device, context.swapchain, UINT64_MAX, context.imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);
    if (acquireResult != VK_SUCCESS) {
        CRITICAL("Image acquiring failed with error code: {}", acquireResult);
    }

    static auto startTime = std::chrono::high_resolution_clock::now();

    auto currentTime = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();
    UniformBufferObject ubo{};
    ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    ubo.proj = glm::perspective(glm::radians(45.0f), context.extent.width / (float) context.extent.height, 0.1f, 10.0f);
    ubo.proj[1][1] *= -1;

    void* data;
    vkMapMemory(context.device, context.uniformBufferMemory, 0, sizeof(ubo), 0, &data);
    memcpy(data, &ubo, sizeof(ubo));
    vkUnmapMemory(context.device, context.uniformBufferMemory);

    vkResetCommandBuffer(context.commandBuffer, 0);
    context.RecordCommandBuffer(context.commandBuffer, imageIndex);

    VkSubmitInfo submitInfo{};
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = &context.imageAvailableSemaphore;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &context.commandBuffer;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &context.renderFinishedSemaphore;

    VkResult submitResult = vkQueueSubmit(context.graphics, 1, &submitInfo, context.inFlightFence);
    if (submitResult != VK_SUCCESS) {
        CRITICAL("Failed to submit draw command buffer with error code: {}", submitResult);
    }

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &context.renderFinishedSemaphore;
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &context.swapchain;
    presentInfo.pImageIndices = &imageIndex;
    VkResult presentResult = vkQueuePresentKHR(context.present, &presentInfo);
    if (presentResult != VK_SUCCESS) {
        CRITICAL("Vulkan presentation failed with error code: {}", presentResult);
    }
}