#pragma once

#include "core/core.h"
#include "vulkan/vulkan.h"
#include <vulkan/vulkan_core.h>

enum ShaderType {
    VERTEX = VK_SHADER_STAGE_VERTEX_BIT,
    FRAGMENT = VK_SHADER_STAGE_FRAGMENT_BIT
};

struct Shader {
    Shader(VkDevice device, const char* fileName, ShaderType type);
    ~Shader() = default;

    void Destroy() { vkDestroyShaderModule(this->device, this->module, nullptr); }
    VkPipelineShaderStageCreateInfo GetStageInfo();

    VkDevice device;
    VkShaderModule module;
    ShaderType type;
};