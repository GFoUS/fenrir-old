#include "renderer.h"

#define GLM_FORCE_RADIANS
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtx/rotate_vector.hpp"
#include "chrono"
#include "core/core.h"
#include "vulkan/uniform.h"
#include "fengui.h"
#include "vulkan/pipeline.h"

const std::vector<Vertex> vertices = {
    {{-0.5f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}},
    {{0.5f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}},
    {{0.5f, 0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}},
    {{-0.5f, 0.5f, 0.0f}, {1.0f, 1.0f, 1.0f}}
};

const std::vector<uint32_t> indices = {
    0, 1, 2, 2, 3, 0
};

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
    imguiInfo.MSAASamples = context.msaaSamples;
    ImGui_ImplVulkan_Init(&imguiInfo, context.pipeline->renderPass);

    context.StartAndSubmitCommandBuffer(context.graphics, [](VkCommandBuffer commandBuffer) {
        ImGui_ImplVulkan_CreateFontsTexture(commandBuffer);
    });
    ImGui_ImplVulkan_DestroyFontUploadObjects();

    this->vertexBuffer.Init(&context, vertices, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    this->indexBuffer.Init(&context, indices, VK_BUFFER_USAGE_INDEX_BUFFER_BIT);

    PipelineBuilder pipelineBuilder(&context);
    Shader vertex(context.device, "shaders/vert.spv", VERTEX);
    Shader fragment(context.device, "shaders/frag.spv", FRAGMENT);
    this->scenePipeline = pipelineBuilder
        .SetDepthTesting(true)
        .SetMsaaSamples(context.msaaSamples)
        .SetShader(&vertex)
        .SetShader(&fragment)
        .SetDynamicViewport()
        .SetResolveLayout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
        .Build();

    this->model = std::make_unique<Model>(&context, "models/samples/2.0/Box/glTF/Box.gltf");
}

Renderer::~Renderer() {
    vkDeviceWaitIdle(context.device);
    INFO("Deleting scene image");
    this->sceneImage->Destroy();
    delete this->sceneImage;
    this->scenePipeline->Destroy();
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

void CreateDockspace() {
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    ImGui::SetNextWindowViewport(viewport->ID);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
    ImGuiDockNodeFlags dockspaceFlags = ImGuiDockNodeFlags_PassthruCentralNode;
    ImGui::Begin("Dockspace", nullptr, windowFlags);
    ImGui::PopStyleVar(2);
    auto dockspaceID = ImGui::GetID("Dockspace");
    ImGui::DockSpace(dockspaceID, ImVec2{ 0, 0 }, dockspaceFlags);
    ImGui::End();
}

void Renderer::OnTick() {
    vkWaitForFences(context.device, 1, &context.inFlightFence, VK_TRUE, UINT64_MAX);
    vkResetFences(context.device, 1, &context.inFlightFence);

    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    
    CreateDockspace();

    ImGui::Begin("Viewport");
    glm::vec2 offset = { ImGui::GetWindowContentRegionMin().x, ImGui::GetWindowContentRegionMin().y };
    offset += glm::vec2{ ImGui::GetWindowPos().x, ImGui::GetWindowPos().y };
    glm::vec2 size = { ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y };
    if (this->sceneImage == nullptr || this->sceneImage->width != size.x || this->sceneImage->height != size.y) {
        if (this->sceneImage != nullptr) {
            delete this->sceneImage;
        }

        if (size.x != 0 && size.y != 0) {
            this->sceneImage = new Image(&context, size.x, size.y, context.format.format, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
        }
    }

    if (this->sceneImage) {
        Image colorImage(&context, size.x, size.y, context.format.format, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, context.msaaSamples);
        Image depthImage(&context, size.x, size.y, VK_FORMAT_D32_SFLOAT, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, context.msaaSamples);
        std::vector<VkImageView> attachments = { colorImage.GetImageView(VK_IMAGE_ASPECT_COLOR_BIT), depthImage.GetImageView(VK_IMAGE_ASPECT_DEPTH_BIT), this->sceneImage->GetImageView(VK_IMAGE_ASPECT_COLOR_BIT) };

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.width = size.x;
        framebufferInfo.height = size.y;
        framebufferInfo.renderPass = this->scenePipeline->renderPass;
        framebufferInfo.attachmentCount = attachments.size();
        framebufferInfo.pAttachments = attachments.data();
        framebufferInfo.layers = 1;
        
        VkFramebuffer framebuffer;
        VkResult framebufferResult = vkCreateFramebuffer(context.device, &framebufferInfo, nullptr, &framebuffer);
        if (framebufferResult != VK_SUCCESS) {
            CRITICAL("Framebuffer creation failed with error code: {}", framebufferResult);
        }

        context.StartAndSubmitCommandBuffer(context.graphics, [this, framebuffer, size](VkCommandBuffer cmd) {
            VkViewport viewport{};
            viewport.width = size.x;
            viewport.height = size.y;
            viewport.x = 0;
            viewport.y = 0;
            viewport.maxDepth = 1.0f;
            viewport.minDepth = 0.0f;

            VkExtent2D extent = { (uint32_t)size.x, (uint32_t)size.y };

            VkRect2D scissor{};
            scissor.offset = { 0, 0 };
            scissor.extent = extent;

            VkRenderPassBeginInfo renderPassInfo{};
            renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            renderPassInfo.renderPass = scenePipeline->renderPass;
            renderPassInfo.framebuffer = framebuffer;
            renderPassInfo.renderArea.offset = { 0, 0 };
            renderPassInfo.renderArea.extent = extent;
            std::array<VkClearValue, 2> clearValues{};
            clearValues[0].color = { {0.0f, 0.0f, 0.0f, 1.0f} };
            clearValues[1].depthStencil = { 1.0f, 0 };
            renderPassInfo.clearValueCount = clearValues.size();
            renderPassInfo.pClearValues = clearValues.data();
            vkCmdBeginRenderPass(cmd, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
            vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, this->scenePipeline->pipeline);

            vkCmdSetViewport(cmd, 0, 1, &viewport);
            vkCmdSetScissor(cmd, 0, 1, &scissor);

            this->model->Render(cmd);

            vkCmdEndRenderPass(cmd);
        });

        vkDestroyFramebuffer(context.device, framebuffer, nullptr);

        if (!this->sceneTexture) {
            this->sceneTexture = ImGui_ImplVulkan_AddTexture(sceneImage->GetSampler(), sceneImage->GetImageView(VK_IMAGE_ASPECT_COLOR_BIT), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        }
        else {
            VkDescriptorImageInfo imageInfo{};
            imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            imageInfo.imageView = this->sceneImage->GetImageView(VK_IMAGE_ASPECT_COLOR_BIT);
            imageInfo.sampler = this->sceneImage->GetSampler();

            VkWriteDescriptorSet textureUpdate{};
            textureUpdate.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            textureUpdate.descriptorCount = 1;
            textureUpdate.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            textureUpdate.dstBinding = 0;
            textureUpdate.dstSet = this->sceneTexture;
            textureUpdate.pImageInfo = &imageInfo;


            vkUpdateDescriptorSets(context.device, 1, &textureUpdate, 0, nullptr);
        }
        ImGui::Image(this->sceneTexture, ImVec2{ size.x, size.y });
    }

    ImGui::End();

    ImGui::ShowMetricsWindow();
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