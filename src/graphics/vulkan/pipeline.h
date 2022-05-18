#pragma once

#include "vulkan/vulkan.h"

#include "core/core.h"

#include "shader.h"
#include "image.h"

struct Context;

struct Pipeline {

	Context* context;

	VkPipeline pipeline;
	VkRenderPass renderPass;
	VkPipelineLayout layout;
	VkImageLayout imageLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkViewport viewport{};
	VkRect2D scissor{};
	VkSampleCountFlagBits msaaSamples = VK_SAMPLE_COUNT_1_BIT;

	void Destroy();
};

class PipelineBuilder {
public:
	PipelineBuilder(Context* context);

	PipelineBuilder SetShader(Shader* shader);
	PipelineBuilder SetViewport(float width, float height);
	PipelineBuilder SetDynamicViewport();
	PipelineBuilder SetMsaaSamples(VkSampleCountFlagBits msaaSamples);
	PipelineBuilder SetDepthTesting(bool depthTesting);
	PipelineBuilder SetResolveLayout(VkImageLayout layout);

	Pipeline* Build();
private:
	Context* context;
	Pipeline* pipeline = new Pipeline();

	std::unordered_map<ShaderType, Shader*> shaders;
	bool depthTesting = true;
	bool usingDynamicViewports = false;
};