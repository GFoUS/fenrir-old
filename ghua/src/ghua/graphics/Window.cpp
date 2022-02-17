#include "Window.h"

namespace GHUA {
    Window::Window(const char* title) {
        if (GLFW_NOT_INITIALIZED) {
            if (!glfwInit()) {
                CRITICAL("GLFW failed to initialise, its all downhill from here...");
                return;
            }
        }

        this->window = glfwCreateWindow(640, 480, title, NULL, NULL);
        if (!this->window) {
            CRITICAL("GLFW window creation failed, nuclear explosion imminent");
            return;
        }

    }

    Window::~Window() {
        glfwTerminate();
    }

    HandleResult Window::HandleEvent(Event* e) {
        INFO("Window recieved: {}", *e);

        return CONTINUE;
    }
}