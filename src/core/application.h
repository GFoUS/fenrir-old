#pragma once

#include "graphics/window.h"
#include "graphics/renderer.h"
#include "layer.h"
#include "event.h"
#include "core.h"

class Application {
public:
    Application();
    ~Application();
    void Run();
private:
    Window window;
    Renderer renderer;
    std::vector<Layer*> layerStack;
    std::vector<Event*> eventBus;
};