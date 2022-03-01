#include "instance.h"
#include "core/core.h"
#include "GLFW/glfw3.h"

const std::vector<const char*> VALIDATION_LAYERS = {
    "VK_LAYER_KHRONOS_validation",
    // "VK_LAYER_LUNARG_api_dump"
};

const std::vector<const char*> EXTENSIONS = {};

struct Context {
    Context();

    Instance instance;

    static std::vector<const char*> FinalizeExtensions() {
        // Append GLFW extensions to the defined list
        std::vector<const char*> extensions = EXTENSIONS;
        uint32_t numGlfwExtensions = 0;
        const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&numGlfwExtensions);
        for (uint32_t i = 0; i < numGlfwExtensions; i++) {
            extensions.push_back(glfwExtensions[i]);
        }

        return extensions;
    }
};