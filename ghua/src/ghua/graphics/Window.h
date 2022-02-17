#pragma once

#include "ghua/core/Core.h"
#include "ghua/core/Layer.h"
#include "GLFW/glfw3.h"

namespace GHUA {
    class Window : public Layer {
    public:
        Window(const char* title);
        ~Window();

        virtual HandleResult HandleEvent(Event* e);
        virtual inline const char* GetName() const {
            return "Window";
        };

        void Tick() {
            glfwSwapBuffers(this->window);
            glfwPollEvents();
        }
    private:
        GLFWwindow* window;
    };
}