#pragma once

#include "core/layer.h"
#include "core/core.h"
#include "core/event.h"

struct GLFWwindow;

class Window : public Layer {
public:
    Window(std::vector<Event*>* eventBus);
    ~Window();

    bool ShouldClose();

    virtual void OnTick();
    virtual void OnEvent(Event* event);
private:
    GLFWwindow* window;
    std::vector<Event*>* eventBus;

    void ErrorCallback(int error, const char* description);
};