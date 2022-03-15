#pragma once

#include "core/core.h"
#include "vulkan/vulkan.h"

namespace tk
{
    enum ShaderType {
        VERTEX = VK_SHADER_STAGE_VERTEX_BIT,
        FRAGMENT = VK_SHADER_STAGE_FRAGMENT_BIT
    };

    class Shader
    {
    public:
        Shader(const char* fileName, ShaderType type, VkDevice device);
        ~Shader();

        void DestroyModule(VkDevice device);
        VkPipelineShaderStageCreateInfo GetStageInfo();
    private:
        VkShaderModule module;
        ShaderType type;
    };
}