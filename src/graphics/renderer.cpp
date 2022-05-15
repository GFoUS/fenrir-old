#include "renderer.h"

#define GLM_FORCE_RADIANS
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtx/rotate_vector.hpp"
#include "chrono"
#include "core/core.h"
#include "vulkan/uniform.h"
#include "vulkan/image.h"
#include "fengui.h"

Renderer::Renderer(Window *window) : context(window) {
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;       // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;           // Enable Docking
    // io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;         // Enable Multi-Viewport / Platform Windows
    ImGui::StyleColorsDark();

    ImGui_ImplGlfw_InitForVulkan(window->GetRawWindow(), true);
    ImGui_ImplVulkan_InitInfo imguiInfo{};
    imguiInfo.Instance = context.instance;
    imguiInfo.PhysicalDevice = context.physical;
    imguiInfo.Device = context.device;
    imguiInfo.QueueFamily = context.queueFamilies.graphics.value();
    imguiInfo.Queue = context.graphics;
    imguiInfo.DescriptorPool = context.descriptorPool;
    imguiInfo.Subpass = 0;
    imguiInfo.MinImageCount = context.images.size();
    imguiInfo.ImageCount = context.images.size();
    imguiInfo.MSAASamples = context.colorImage->samples;
    ImGui_ImplVulkan_Init(&imguiInfo, context.renderPass);

    context.StartAndSubmitCommandBuffer(context.graphics, [](VkCommandBuffer commandBuffer) {
        ImGui_ImplVulkan_CreateFontsTexture(commandBuffer);
    });
    ImGui_ImplVulkan_DestroyFontUploadObjects();
}

Renderer::~Renderer() = default;

void Renderer::OnTick() {
    vkWaitForFences(context.device, 1, &context.inFlightFence, VK_TRUE, UINT64_MAX);
    vkResetFences(context.device, 1, &context.inFlightFence);

    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    ImGui::ShowDemoWindow();
    ImGui::Render();
    ImDrawData* imguiDrawData = ImGui::GetDrawData();

    uint32_t imageIndex;
    VkResult acquireResult = vkAcquireNextImageKHR(context.device, context.swapchain, UINT64_MAX, context.imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);
    if (acquireResult != VK_SUCCESS) {
        CRITICAL("Image acquiring failed with error code: {}", acquireResult);
    }

    static auto startTime = std::chrono::high_resolution_clock::now();

    auto currentTime = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();
    UniformBufferObject ubo{};
    glm::mat4 rotation = glm::rotate(glm::mat4(1.0f), glm::radians(22.5f * time), glm::vec3(0.0f, 0.0f, 1.0f));
    glm::vec3 eye = glm::vec3(glm::vec4(100.0f, 100.0f, 100.0f, 0.0f) * rotation);
    ubo.view = glm::lookAt(eye, glm::vec3(0.0f, 0.0f, 400.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    ubo.proj = glm::perspective(glm::radians(45.0f), context.extent.width / (float) context.extent.height, 1.0f, 10000.0f);
    ubo.proj[1][1] *= -1;

    void* data;
    vkMapMemory(context.device, context.uniformBufferMemory, 0, sizeof(ubo), 0, &data);
    memcpy(data, &ubo, sizeof(ubo));
    vkUnmapMemory(context.device, context.uniformBufferMemory);

    vkResetCommandBuffer(context.commandBuffer, 0);
    context.StartCommandBuffer(context.commandBuffer, imageIndex);
    ImGui_ImplVulkan_RenderDrawData(imguiDrawData, context.commandBuffer);
    context.EndCommandBuffer(context.commandBuffer);

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

    ImGui::UpdatePlatformWindows();
    ImGui::RenderPlatformWindowsDefault();

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