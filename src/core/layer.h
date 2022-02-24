#pragma once

#include "event.h"

class Layer {
public:
    virtual void OnAttach() {};
    virtual void OnDetach() {};
    virtual void OnTick() {};
    virtual void OnEvent(Event* event) {};
};