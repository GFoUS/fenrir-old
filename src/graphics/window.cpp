#include "window.h"
#define GLFW_INCLUDE_VULKAN
#include "GLFW/glfw3.h"
#include "windowevents.h"

Window::Window(std::vector<Event*>* eventBus) {
    if (!glfwInit()) {
        CRITICAL("GLFW failed to initialise, all downhill from here...");
    }

    glfwSetErrorCallback([](int error, const char* description) {
        fprintf(stderr, "GLFW Error: %s\n", description);
    });

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    this->window = glfwCreateWindow(WIDTH, HEIGHT, "FENRIR", NULL, NULL);
    if (!this->window) {
        glfwTerminate();
        CRITICAL("Window creation failed, explosion imminent");
    }

    glfwSetWindowUserPointer(this->window, eventBus);
    glfwSetWindowCloseCallback(this->window, [](GLFWwindow* window) {
        auto* eventBus = (std::vector<Event*>*)glfwGetWindowUserPointer(window);
        eventBus->push_back(new WindowCloseEvent());
    });

    glfwSetKeyCallback(this->window, [](GLFWwindow* window, int key, int scancode, int action, int mods) {
        auto* eventBus = (std::vector<Event*>*)glfwGetWindowUserPointer(window);
        eventBus->push_back(new KeyEvent(key, action));
    });
}

Window::~Window() {
    glfwDestroyWindow(this->window);
    glfwTerminate();
}

void Window::OnTick() {
    glfwPollEvents();
}

void Window::OnEvent(Event* event) {
    if (event->GetType() == EventType::WINDOW_CLOSE) glfwSetWindowShouldClose(this->window, true);
    if (event->GetType() == EventType::KEY) {
        auto* keyEvent = (KeyEvent*)event;
        if (keyEvent->key == GLFW_KEY_ESCAPE) {
            glfwSetWindowShouldClose(this->window, true);
        }
    }
}

bool Window::ShouldClose() {
    return glfwWindowShouldClose(this->window);
}
