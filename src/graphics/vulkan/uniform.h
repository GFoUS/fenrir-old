#pragma once

#include "core/core.h"
#include "vulkan/vulkan.h"
#include "glm/glm.hpp"

struct UniformBufferObject {
    glm::mat4 view;
    glm::mat4 proj;
};
