#include "shader.h"

#include "fstream"

namespace tk
{
    Shader::Shader(const char *fileName, ShaderType type, VkDevice device) : type(type)
    {
        // READ SHADER CODE
        std::ifstream file(fileName, std::ios::ate | std::ios::binary);
        if (!file.is_open())
        {
            CRITICAL("File with name {} failed to open", fileName);
        }
        size_t fileSize = (size_t)file.tellg();
        std::vector<char> code(fileSize);
        file.seekg(0);
        file.read(code.data(), fileSize);
        file.close();

        VkShaderModuleCreateInfo createInfo{};
        createInfo.codeSize = code.size();
        createInfo.flags = 0;
        createInfo.pCode = reinterpret_cast<const uint32_t *>(code.data());
        createInfo.pNext = nullptr;
        createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;

        VkResult result = vkCreateShaderModule(device, &createInfo, nullptr, &this->module);
        if (result != VK_SUCCESS)
        {
            CRITICAL("Shader module creation for file {} failed", fileName);
        }

        INFO("Loaded shader: {}", fileName);
    }

    void Shader::DestroyModule(VkDevice device)
    {
        vkDestroyShaderModule(device, this->module, nullptr);
    }

    VkPipelineShaderStageCreateInfo Shader::GetStageInfo()
    {
        VkPipelineShaderStageCreateInfo stageInfo{};
        stageInfo.flags = 0;
        stageInfo.module = this->module;
        stageInfo.pName = "main";
        stageInfo.pSpecializationInfo = nullptr;
        stageInfo.stage = (VkShaderStageFlagBits)this->type;
        stageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        return stageInfo;
    }

    Shader::~Shader()
    {
    }
}