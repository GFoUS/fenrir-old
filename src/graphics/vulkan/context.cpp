#include "context.h"
#include "instance.h"
#include "physical.h"
#include "device.h"
#include "surface.h"
#include "renderpass.h"
#include "layout.h"
#include "pipeline.h"
#include "shader.h"
#include "framebuffer.h"
#include "command.h"

Context::Context(Window *window)
{
    this->window = window;
    this->instance = tk::GetInstance();
    INFO("Instance created");
    this->surface = tk::GetSurface(this->instance, this->window);
    INFO("Surface created");
    this->physical = tk::PickPhysical(this->instance, this->surface);
    INFO("Physical device picked");
    this->device = tk::GetDevice(this->physical, this->surface);
    INFO("Logical device created");
    this->graphicsQueue = tk::GetGraphicsQueue(this->physical, this->device);
    INFO("Got graphics queue");

    this->presentQueue = tk::GetPresentQueue(this->physical, this->device, this->surface);
    INFO("Got present queue");
    this->swapchain = tk::GetSwapchain(this->physical, this->surface, this->device, this->window->GetRawWindow());
    INFO("Swapchain created");
    this->images = tk::GetSwapchainImages(this->device, this->swapchain.swapchain);
    INFO("Created images");
    this->imageViews = tk::GetSwapchainImageViews(this->device, this->images, this->swapchain.format);
    INFO("Created image views");

    tk::Shader vertex("shaders/vert.spv", tk::ShaderType::VERTEX, this->device);
    tk::Shader fragment("shaders/frag.spv", tk::ShaderType::FRAGMENT, this->device);
    std::vector<VkPipelineShaderStageCreateInfo> shaderStages = {vertex.GetStageInfo(), fragment.GetStageInfo()};
    INFO("Created shaders");

    this->renderPass = tk::GetRenderPass(this->device, this->swapchain);
    INFO("Created render pass");
    this->layout = tk::GetPipelineLayout(this->device);
    INFO("Created pipeline layout");
    this->pipeline = tk::GetPipeline(this->device, this->swapchain, shaderStages, this->layout, this->renderPass);
    INFO("Created pipeline");
    vertex.DestroyModule(this->device);
    fragment.DestroyModule(this->device);

    this->framebuffers = tk::GetFramebuffers(this->device, this->imageViews, this->renderPass, this->swapchain.extent);
    INFO("Created framebuffers");

    this->commandPool = tk::CreateCommandPool(this->device, this->physical);
    this->buffer = tk::CreateCommandBuffer(this->device, this->commandPool);
    INFO("Created command buffer");

    this->CreateSyncObjects();
}

Context::~Context()
{
    vkDeviceWaitIdle(this->device);

    vkDestroySemaphore(this->device, this->imageAvailable, nullptr);
    vkDestroySemaphore(this->device, this->renderFinished, nullptr);
    vkDestroyFence(this->device, this->inFlight, nullptr);

    vkDestroyCommandPool(this->device, this->commandPool, nullptr);
    for (auto &framebuffer : this->framebuffers)
    {
        vkDestroyFramebuffer(this->device, framebuffer, nullptr);
    }
    vkDestroyPipeline(this->device, this->pipeline, nullptr);
    vkDestroyPipelineLayout(this->device, this->layout, nullptr);
    vkDestroyRenderPass(this->device, this->renderPass, nullptr);
    for (auto &imageView : this->imageViews)
    {
        vkDestroyImageView(this->device, imageView, nullptr);
    }
    vkDestroySwapchainKHR(this->device, this->swapchain.swapchain, nullptr);
    vkDestroySurfaceKHR(this->instance, this->surface, nullptr);
    vkDestroyDevice(this->device, nullptr);
    vkDestroyInstance(this->instance, nullptr);
}

void Context::DrawFrame()
{
    vkWaitForFences(this->device, 1, &this->inFlight, VK_TRUE, UINT64_MAX);
    vkResetFences(this->device, 1, &this->inFlight);

    uint32_t imageIndex;
    VkResult imageAquired = vkAcquireNextImageKHR(this->device, this->swapchain.swapchain, UINT64_MAX, this->imageAvailable, nullptr, &imageIndex);
    if (imageAquired != VK_SUCCESS)
    {
        CRITICAL("Failed to aquire new image with error code: {}", imageAquired);
    }

    vkResetCommandBuffer(this->buffer, 0);
    tk::RecordCommandBuffer(this->buffer, this->framebuffers[imageIndex], this->swapchain.extent, this->renderPass, this->pipeline);

    VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

    VkSubmitInfo submitInfo{};
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &this->buffer;
    submitInfo.pNext = nullptr;
    submitInfo.pSignalSemaphores = &this->renderFinished;
    submitInfo.pWaitDstStageMask = &waitStage;
    submitInfo.pWaitSemaphores = &this->imageAvailable;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.waitSemaphoreCount = 1;

    VkResult submitted = vkQueueSubmit(this->graphicsQueue, 1, &submitInfo, this->inFlight);
    if (submitted != VK_SUCCESS)
    {
        CRITICAL("Failed to submit command buffer with error code: {}", submitted);
    }

    VkPresentInfoKHR presentInfo{};
    presentInfo.pImageIndices = &imageIndex;
    presentInfo.pNext = nullptr;
    presentInfo.pResults = nullptr;
    presentInfo.pSwapchains = &this->swapchain.swapchain;
    presentInfo.pWaitSemaphores = &this->renderFinished;
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.swapchainCount = 1;
    presentInfo.waitSemaphoreCount = 1;
    VkResult presented = vkQueuePresentKHR(this->presentQueue, &presentInfo);
    if (presented != VK_SUCCESS)
    {
        CRITICAL("Failed to present with error code: {}", presented);
    }
}

void Context::CreateSyncObjects()
{
    VkSemaphoreCreateInfo semInfo{};
    semInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    VkResult sem1 = vkCreateSemaphore(this->device, &semInfo, nullptr, &this->imageAvailable);
    VkResult sem2 = vkCreateSemaphore(this->device, &semInfo, nullptr, &this->renderFinished);
    VkResult fence = vkCreateFence(this->device, &fenceInfo, nullptr, &this->inFlight);

    if (sem1 != VK_SUCCESS || sem2 != VK_SUCCESS)
    {
        CRITICAL("Semaphore creation failed with error code: {}", sem1); // Chances are that they failed for the same reason
    }

    if (fence != VK_SUCCESS)
    {
        CRITICAL("Fence creation failed with error code: {}", fence);
    }
}