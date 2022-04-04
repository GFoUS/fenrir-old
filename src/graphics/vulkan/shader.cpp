#include "shader.h"

#include "fstream"
#include <vulkan/vulkan_core.h>

Shader::Shader(VkDevice device, const char* fileName, ShaderType type) : device(device), type(type) {
    std::ifstream file(fileName, std::ios::ate | std::ios::binary);
    if (!file.is_open()) {
        CRITICAL("Opening shader file {} failed", fileName);
    }
    size_t fileSize = file.tellg();
    std::vector<char> buffer(fileSize);
    file.seekg(0);
    file.read(buffer.data(), fileSize);
    file.close();

    VkShaderModuleCreateInfo moduleInfo{};
    moduleInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    moduleInfo.codeSize = buffer.size();
    moduleInfo.pCode = reinterpret_cast<const uint32_t*>(buffer.data());

    VkResult result = vkCreateShaderModule(device, &moduleInfo, nullptr, &this->module);
    if (result != VK_SUCCESS) {
        CRITICAL("Failed to create shader module with error code: {}", result);
    }
}

VkPipelineShaderStageCreateInfo Shader::GetStageInfo() {
    VkPipelineShaderStageCreateInfo stageInfo{};
    stageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stageInfo.pName = "main";
    stageInfo.module = this->module;
    stageInfo.stage = (VkShaderStageFlagBits)this->type;

    return stageInfo;
}
