#include "renderer.h"

#include "core/core.h"

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