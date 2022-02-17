#pragma once

#include "Core.h"
#include "Event.h"
#include "Layer.h"
#include "ghua/graphics/Window.h"

#define BIND_EVENT_FN(f) std::bind(&Application::f, this, std::placeholders::_1)

namespace GHUA {
    class Application {
    public:
        Application(const char* title);
        virtual ~Application();

        void Run();

        void PushEvent(Event* e) {
            this->eventBus.push_back(e);
        };

        void RegisterLayer(Layer* l) {
            l->SetEventPushCallback(BIND_EVENT_FN(PushEvent));
            this->layerStack.push_back(l);
        };
    private:
       std::vector<Event*> eventBus; 
       std::vector<Layer*> layerStack;

       Window* window;
    };
}