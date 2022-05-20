#pragma once

#include "core/core.h"
#include "vulkan/vulkan.h"

struct Context;

class DescriptorAllocator {
public:
	DescriptorAllocator();
	~DescriptorAllocator();
private:
	typedef PoolSizes = std::unordered_map<VkDescriptorType, uint32_t>;

	struct DescriptorPool {
		VkDescriptorPool pool;
		PoolSizes remaining;
		std::vector<VkDescriptorSet> allocated;
	};

	struct DescriptorSet {
		VkDescriptorSet set;
		VkDescriptorSetLayout layout;
	};

	std::vector<DescriptorPools> pools;
};