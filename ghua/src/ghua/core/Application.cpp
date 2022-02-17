#include "Application.h"
#include "iostream"
#include "algorithm"

namespace GHUA {
    Application::Application(const char* title) : window(new Window(title)) {
        return;
    }

    Application::~Application() {
        
    }

    void Application::Run() {
        INFO("Application running")
        while (true) {
            // Handle any events in the event bus
            std::vector<Layer*> reversedLayerStack;
            reversedLayerStack.resize(this->layerStack.size());
            std::reverse_copy(this->layerStack.begin(), this->layerStack.end(), reversedLayerStack.begin());

            for (Event* event : this->eventBus) {
                INFO("Handling Event: {}", *event);
                for (Layer* layer : reversedLayerStack) {
                    INFO("Dispatching to Layer: {}", *layer);
                    if (layer->HandleEvent(event)) {
                        continue; // If HandleEvent is positive, do not continue handling the event
                    }
                }
            }
        }
    }
}
