#include "pipeline.h"

namespace tk
{
    VkPipeline GetPipeline(VkDevice device, Swapchain swapchain, std::vector<VkPipelineShaderStageCreateInfo> shaders, VkPipelineLayout layout, VkRenderPass renderPass)
    {
        auto vertexInputInfo = _GetVertexInputInfo();
        auto inputAssemblyInfo = _GetInputAssemblyInfo();
        auto viewportInfo = _GetViewportInfo(swapchain.extent);
        auto rasterInfo = _GetRasterizationInfo();
        auto multisamplingInfo = _GetMultisamplingInfo();
        auto blendInfo = _GetBlendStateInfo();

        VkGraphicsPipelineCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        createInfo.stageCount = shaders.size();
        createInfo.pStages = shaders.data();

        createInfo.pVertexInputState = &vertexInputInfo;
        createInfo.pInputAssemblyState = &inputAssemblyInfo;
        createInfo.pViewportState = &viewportInfo;
        createInfo.pRasterizationState = &rasterInfo;
        createInfo.pMultisampleState = &multisamplingInfo;
        createInfo.pDepthStencilState = nullptr;
        createInfo.pColorBlendState = &blendInfo;
        createInfo.pDynamicState = nullptr;

        createInfo.layout = layout;

        createInfo.renderPass = renderPass;
        createInfo.subpass = 0;

        createInfo.basePipelineHandle = VK_NULL_HANDLE;
        createInfo.basePipelineIndex = -1;

        VkPipeline pipeline;
        VkResult result = vkCreateGraphicsPipelines(device, nullptr, 1, &createInfo, nullptr, &pipeline);
        if (result != VK_SUCCESS)
        {
            CRITICAL("Pipeline creation failed with error code: {}", result);
        }

        delete viewportInfo.pViewports;
        delete viewportInfo.pScissors;
        delete blendInfo.pAttachments;

        return pipeline;
    }

    VkPipelineVertexInputStateCreateInfo _GetVertexInputInfo()
    {
        VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
        vertexInputInfo.flags = 0;
        vertexInputInfo.pNext = nullptr;
        vertexInputInfo.pVertexAttributeDescriptions = nullptr;
        vertexInputInfo.pVertexBindingDescriptions = nullptr;
        vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertexInputInfo.vertexAttributeDescriptionCount = 0;
        vertexInputInfo.vertexBindingDescriptionCount = 0;

        return vertexInputInfo;
    }

    VkPipelineInputAssemblyStateCreateInfo _GetInputAssemblyInfo()
    {
        VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo{};
        inputAssemblyInfo.flags = 0;
        inputAssemblyInfo.pNext = nullptr;
        inputAssemblyInfo.primitiveRestartEnable = VK_FALSE;
        inputAssemblyInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

        return inputAssemblyInfo;
    }

    VkPipelineViewportStateCreateInfo _GetViewportInfo(VkExtent2D extent)
    {
        auto viewport = new VkViewport();;
        viewport->x = 0.0f;
        viewport->y = 0.0f;
        viewport->width = (float)extent.width;
        viewport->height = (float)extent.height;
        viewport->minDepth = 0.0f;
        viewport->maxDepth = 1.0f;

        auto scissor = new VkRect2D();
        scissor->offset = {0, 0};
        scissor->extent = extent;

        VkPipelineViewportStateCreateInfo viewportInfo;
        viewportInfo.flags = 0;
        viewportInfo.pNext = nullptr;
        viewportInfo.pScissors = scissor;
        viewportInfo.pViewports = viewport;
        viewportInfo.scissorCount = 1;
        viewportInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportInfo.viewportCount = 1;

        return viewportInfo;
    }

    VkPipelineRasterizationStateCreateInfo _GetRasterizationInfo()
    {
        VkPipelineRasterizationStateCreateInfo rasterInfo{};
        rasterInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterInfo.depthClampEnable = VK_FALSE;
        rasterInfo.rasterizerDiscardEnable = VK_FALSE;
        rasterInfo.polygonMode = VK_POLYGON_MODE_FILL;
        rasterInfo.lineWidth = 1.0f;
        rasterInfo.cullMode = VK_CULL_MODE_BACK_BIT;
        rasterInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;
        rasterInfo.depthBiasEnable = VK_FALSE;
        rasterInfo.depthBiasConstantFactor = 0.0f;
        rasterInfo.depthBiasClamp = 0.0f;
        rasterInfo.depthBiasSlopeFactor = 0.0f;
        return rasterInfo;
    }

    VkPipelineMultisampleStateCreateInfo _GetMultisamplingInfo()
    {
        VkPipelineMultisampleStateCreateInfo multisamplingInfo{};
        multisamplingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisamplingInfo.sampleShadingEnable = VK_FALSE;
        multisamplingInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
        multisamplingInfo.minSampleShading = 1.0f;
        multisamplingInfo.pSampleMask = nullptr;
        multisamplingInfo.alphaToCoverageEnable = VK_FALSE;
        multisamplingInfo.alphaToOneEnable = VK_FALSE;
        return multisamplingInfo;
    }

    VkPipelineColorBlendStateCreateInfo _GetBlendStateInfo()
    {
        auto colourBlendAttachmentInfo = new VkPipelineColorBlendAttachmentState();
        colourBlendAttachmentInfo->colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        colourBlendAttachmentInfo->blendEnable = VK_FALSE;
        colourBlendAttachmentInfo->srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
        colourBlendAttachmentInfo->dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
        colourBlendAttachmentInfo->colorBlendOp = VK_BLEND_OP_ADD;
        colourBlendAttachmentInfo->srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        colourBlendAttachmentInfo->dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        colourBlendAttachmentInfo->alphaBlendOp = VK_BLEND_OP_ADD;

        VkPipelineColorBlendStateCreateInfo colourBlendingInfo{};
        colourBlendingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colourBlendingInfo.logicOpEnable = VK_FALSE;
        colourBlendingInfo.logicOp = VK_LOGIC_OP_COPY;
        colourBlendingInfo.attachmentCount = 1;
        colourBlendingInfo.pAttachments = colourBlendAttachmentInfo;
        colourBlendingInfo.blendConstants[0] = 0.0f;
        colourBlendingInfo.blendConstants[1] = 0.0f;
        colourBlendingInfo.blendConstants[2] = 0.0f;
        colourBlendingInfo.blendConstants[3] = 0.0f;
        return colourBlendingInfo;
    }
}