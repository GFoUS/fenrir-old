#include "pipeline.h"

PipelineBuilder::PipelineBuilder(Context* context) : context(context) {
    // TODO: Default shaders
    this->pipeline = new Pipeline();
    this->pipeline->context = context;
    this->shaders[VERTEX] = nullptr;
    this->shaders[FRAGMENT] = nullptr;
}

PipelineBuilder PipelineBuilder::SetShader(Shader *shader)
{
    this->shaders[shader->type] = shader;
    return *this;
}

PipelineBuilder PipelineBuilder::SetViewport(float width, float height)
{
    pipeline->viewport.x = 0.0f;
    pipeline->viewport.y = 0.0f;
    pipeline->viewport.width = width;
    pipeline->viewport.height = height;
    pipeline->viewport.minDepth = 0.0f;
    pipeline->viewport.maxDepth = 1.0f;

    VkExtent2D extent{};
    extent.width = width;
    extent.height = height;

    pipeline->scissor.offset = { 0, 0 };
    pipeline->scissor.extent = extent;

    return *this;
}

PipelineBuilder PipelineBuilder::SetDynamicViewport()
{
    this->usingDynamicViewports = true;
    return *this;
}

PipelineBuilder PipelineBuilder::SetMsaaSamples(VkSampleCountFlagBits msaaSamples) {
    pipeline->msaaSamples = msaaSamples;
    return *this;
}

PipelineBuilder PipelineBuilder::SetDepthTesting(bool depthTesting)
{
    this->depthTesting = true;
    return *this;
}

PipelineBuilder PipelineBuilder::SetResolveLayout(VkImageLayout layout)
{
    this->pipeline->imageLayout = layout;
    return *this;
}

Pipeline* PipelineBuilder::Build() {
    if (!this->shaders[VERTEX]) {
        CRITICAL("Pipeline missing vertex shader");
    }
    if (!this->shaders[FRAGMENT]) {
        CRITICAL("Pipeline missing fragment shader");
    }
    VkPipelineShaderStageCreateInfo shaderStages[2] = { this->shaders[VERTEX]->GetStageInfo(), this->shaders[FRAGMENT]->GetStageInfo() };

    auto bindingDescription = Vertex::GetBindingDescription();
    auto attributeDescriptions = Vertex::GetAttributeDescriptions();
    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
    vertexInputInfo.vertexAttributeDescriptionCount = attributeDescriptions.size();
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports = &pipeline->viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &pipeline->scissor;

    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;

    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = pipeline->msaaSamples;

    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;

    VkPipelineDepthStencilStateCreateInfo depthStencil{};
    if (this->depthTesting) {
        depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depthStencil.depthTestEnable = VK_TRUE;
        depthStencil.depthWriteEnable = VK_TRUE;
        depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
        depthStencil.depthBoundsTestEnable = VK_FALSE;
        depthStencil.depthBoundsTestEnable = VK_FALSE;
        depthStencil.stencilTestEnable = VK_FALSE;
    }

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 0;

    VkResult layoutResult = vkCreatePipelineLayout(context->device, &pipelineLayoutInfo, nullptr, &pipeline->layout);
    if (layoutResult != VK_SUCCESS) {
        CRITICAL("Vulkan pipeline layout creation failed with error code: {}", layoutResult);
    }

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;

    std::vector<VkAttachmentDescription> attachments;

    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = context->format.format;
    colorAttachment.samples = pipeline->msaaSamples;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    attachments.push_back(colorAttachment);

    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    subpass.pColorAttachments = &colorAttachmentRef;

    if (this->depthTesting) {
        VkAttachmentDescription depthAttachment{};
        depthAttachment.format = VK_FORMAT_D32_SFLOAT;
        depthAttachment.samples = pipeline->msaaSamples;
        depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        attachments.push_back(depthAttachment);

        VkAttachmentReference depthAttachmentRef{};
        depthAttachmentRef.attachment = 1;
        depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        subpass.pDepthStencilAttachment = &depthAttachmentRef;
    }
    
    VkAttachmentDescription colorAttachmentResolve{};
    if (pipeline->msaaSamples != VK_SAMPLE_COUNT_1_BIT) {
        colorAttachmentResolve.format = context->format.format; // Will always resolve to format used for swapchain
        colorAttachmentResolve.samples = VK_SAMPLE_COUNT_1_BIT;
        colorAttachmentResolve.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachmentResolve.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachmentResolve.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachmentResolve.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachmentResolve.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachmentResolve.finalLayout = pipeline->imageLayout;
        attachments.push_back(colorAttachmentResolve);

        VkAttachmentReference colorAttachmentResolveRef{};
        colorAttachmentResolveRef.attachment = 2;
        colorAttachmentResolveRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        subpass.pResolveAttachments = &colorAttachmentResolveRef;
    }

    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = attachments.size();
    renderPassInfo.pAttachments = attachments.data();
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;

    VkResult renderPassResult = vkCreateRenderPass(context->device, &renderPassInfo, nullptr, &pipeline->renderPass);
    if (renderPassResult != VK_SUCCESS) {
        CRITICAL("Vulkan render pass creation failed with error code: {}", renderPassResult);
    }

    std::vector<VkDynamicState> dynamicState;
    if (this->usingDynamicViewports) {
        dynamicState.push_back(VK_DYNAMIC_STATE_VIEWPORT);
        dynamicState.push_back(VK_DYNAMIC_STATE_SCISSOR);
    }
    VkPipelineDynamicStateCreateInfo dynamicInfo{};
    dynamicInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicInfo.dynamicStateCount = dynamicState.size();
    dynamicInfo.pDynamicStates = dynamicState.data();

    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDepthStencilState = &depthStencil;
    pipelineInfo.pDynamicState = &dynamicInfo;
    pipelineInfo.layout = pipeline->layout;
    pipelineInfo.renderPass = pipeline->renderPass;
    pipelineInfo.subpass = 0;

    VkResult pipelineResult = vkCreateGraphicsPipelines(context->device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline->pipeline);
    if (pipelineResult != VK_SUCCESS) {
        CRITICAL("Vulkan pipeline creation failed with error code: {}", pipelineResult);
    }

    for (auto& shader : this->shaders) shader.second->Destroy();

    return pipeline;
}

void Pipeline::Destroy() {
    vkDestroyPipeline(context->device, this->pipeline, nullptr);
    vkDestroyRenderPass(context->device, this->renderPass, nullptr);
    vkDestroyPipelineLayout(context->device, this->layout, nullptr);

    delete this;
}
