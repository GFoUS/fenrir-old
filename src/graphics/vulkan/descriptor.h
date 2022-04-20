#pragma once

#include "core/core.h"
#include "context.h"
#include "buffer.h"
#include "image.h"
#include "vulkan/vulkan.h"
#include "vma.h"
#include <vulkan/vulkan_core.h>

const uint32_t POOL_AMOUNT = 1000;

// TODO: Refactor this so that it calculates the required pool sizes instead of using the ratios
class DescriptorAllocator {
public:
    void Init(Context* context) {
        _context = context;
    }

    void Cleanup() {
        for (auto& pool : _freePools) {
            vkDestroyDescriptorPool(_context->device, pool, nullptr);
        }

        for (auto& pool : _usedPools) {
            vkDestroyDescriptorPool(_context->device, pool, nullptr);
        }
    }


    std::vector<VkDescriptorSet> Allocate(VkDescriptorSetLayout layout, uint32_t amount) {
        if (_currentPool == nullptr) {
            _currentPool = GrabPool();
        }

        std::vector<VkDescriptorSetLayout> layouts(amount, layout);
        VkDescriptorSetAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.pSetLayouts = layouts.data();
		allocInfo.descriptorPool = _currentPool;
		allocInfo.descriptorSetCount = amount;

        std::vector<VkDescriptorSet> sets(amount);
        VkResult allocResult = vkAllocateDescriptorSets(_context->device, &allocInfo, sets.data());
		bool needReallocate = false;

        switch (allocResult) {
		case VK_SUCCESS:
			return sets;
		case VK_ERROR_FRAGMENTED_POOL:
		case VK_ERROR_OUT_OF_POOL_MEMORY:
			needReallocate = true;
			break;
		default:
			CRITICAL("Descriptor set allocation failed with error code: {}", allocResult);
		}

		if (needReallocate) {
			// Allocate a new pool and retry
			_currentPool = GrabPool();

			allocResult = vkAllocateDescriptorSets(_context->device, &allocInfo, sets.data());

			// If it still fails then we have big issues
			if (allocResult != VK_SUCCESS) {
			    CRITICAL("Descriptor set allocation failed with error code: {}", allocResult);
            }
            return sets;
        }

        CRITICAL("Should not reach this");
    }

private:
    struct PoolSizes {
		std::vector<std::pair<VkDescriptorType, float>> sizes =
		{
			{ VK_DESCRIPTOR_TYPE_SAMPLER, 0.5f },
			{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 4.f },
			{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 4.f },
			{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1.f },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1.f },
			{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1.f },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2.f },
			{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 2.f },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1.f },
			{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1.f },
			{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 0.5f }
	    };
	};

    VkDescriptorPool CreatePool(const DescriptorAllocator::PoolSizes &poolSizes, int count, VkDescriptorPoolCreateFlags flags)
	{
		std::vector<VkDescriptorPoolSize> sizes;
		sizes.reserve(poolSizes.sizes.size());
		for (auto [type, ratio] : poolSizes.sizes) {
			sizes.push_back({ type, uint32_t(ratio * count) });
		}
		VkDescriptorPoolCreateInfo poolInfo = {};
		poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.flags = flags;
		poolInfo.maxSets = count;
		poolInfo.poolSizeCount = sizes.size();
		poolInfo.pPoolSizes = sizes.data();

		VkDescriptorPool descriptorPool;
		vkCreateDescriptorPool(_context->device, &poolInfo, nullptr, &descriptorPool);

		return descriptorPool;
	}

    VkDescriptorPool GrabPool() {
        if (_freePools.size() > 0) {
            VkDescriptorPool pool = _freePools.back();
            _freePools.pop_back();
            _usedPools.push_back(pool);
            return pool;
        } else {
            return CreatePool(_poolSizes, 1000, 0);
        }
    }

    Context* _context{};
    VkDescriptorPool _currentPool{};
    std::vector<VkDescriptorPool> _freePools{};
    std::vector<VkDescriptorPool> _usedPools{};
    PoolSizes _poolSizes{};
};

// TODO: Cache descriptor set layouts
class DescriptorBuilder {
public:
    static DescriptorBuilder Begin(Context* context, DescriptorAllocator *allocator) {
        DescriptorBuilder builder;
        builder._context = context;
        builder._allocator = allocator;
        return builder;
    }

    DescriptorBuilder BindBuffer(uint32_t bindingIdx, VkDescriptorBufferInfo &bufferInfo, VkDescriptorType type, VkShaderStageFlags stages) {
        VkDescriptorSetLayoutBinding binding{};
        binding.descriptorCount = 1;
		binding.descriptorType = type;
		binding.pImmutableSamplers = nullptr;
		binding.stageFlags = stages;
		binding.binding = bindingIdx;
        _bindings.push_back(binding);

        VkWriteDescriptorSet write{};
        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		write.descriptorCount = 1;
		write.descriptorType = type;
		write.pBufferInfo = &bufferInfo;
		write.dstBinding = bindingIdx;
        _writes.push_back(write);

        return *this;
    }

    DescriptorBuilder BindImage(uint32_t bindingIdx, VkDescriptorImageInfo &imageInfo, VkDescriptorType type, VkShaderStageFlags stages) {
        VkDescriptorSetLayoutBinding binding{};
        binding.descriptorCount = 1;
		binding.descriptorType = type;
		binding.pImmutableSamplers = nullptr;
		binding.stageFlags = stages;
		binding.binding = bindingIdx;
        _bindings.push_back(binding);

        VkWriteDescriptorSet write{};
        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		write.descriptorCount = 1;
		write.descriptorType = type;
		write.pImageInfo = &imageInfo;
		write.dstBinding = bindingIdx;
        _writes.push_back(write);

        return *this;
    }

    // TODO: bool Build(VkDescriptorSet &set, VkDescriptorSetLayout &layout);
    VkDescriptorSet Build() {
        VkDescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.pBindings = _bindings.data();
        layoutInfo.bindingCount = _bindings.size();
        VkResult layoutResult = vkCreateDescriptorSetLayout(_context->device, &layoutInfo, nullptr, &_layout);
        if (layoutResult != VK_SUCCESS) {
            CRITICAL("Descriptor layout creation failed with error code: {}", layoutResult);
        }
        
        return _allocator->Allocate(_layout, 1).front();   
    }
private:
    Context* _context;
    DescriptorAllocator *_allocator;
    std::vector<VkDescriptorSetLayoutBinding> _bindings;
    std::vector<VkWriteDescriptorSet> _writes;

    VkDescriptorSet _set;
    VkDescriptorSetLayout _layout;
};