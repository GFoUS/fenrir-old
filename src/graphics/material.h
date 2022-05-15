#pragma once

#include "vulkan/shader.h"
#include "vulkan/vulkan.h"

class Material {
public:
	Material(std::shared_ptr<Shader> shader);
	~Material();

	void Use(VkDescriptorSet set);

	std::shared_ptr<Shader> shader;
	VkPipeline pipeline;
};