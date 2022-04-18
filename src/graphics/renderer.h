#pragma once

#include "vulkan/context.h"
#include "window.h"
#include "model.h"

class Renderer : public Layer {
public:
    Renderer(Window* window);
    ~Renderer();

    virtual void OnTick() override;
private:
    Context context;
    Model model;
};