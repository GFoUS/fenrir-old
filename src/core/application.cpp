#include "application.h"

#include "algorithm"

Application::Application() : window(&this->eventBus), renderer(&this->window) {
    this->layerStack.push_back(&this->window);
    this->layerStack.push_back(&this->renderer);
}

void Application::Run() {
    while (true) {
        if (this->window.ShouldClose()) return;

        for (Layer* layer : this->layerStack) {
            layer->OnTick();
        }

        while (this->eventBus.size() != 0) {
            for (int i = this->layerStack.size() - 1; i >= 0; i--) {
                this->layerStack[i]->OnEvent(this->eventBus[0]);
            }
            delete this->eventBus[0]; // Free the event off the heap
            this->eventBus.erase(this->eventBus.begin()); // Remove the pointer from the bus
        } 
    }
}

Application::~Application() {}