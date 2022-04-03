#pragma once

#include "vulkan/context.h"
#include "window.h"

class Renderer : public Layer {
public:
    Renderer(Window* window);
    ~Renderer();

    virtual void OnTick() override;
private:
    void CreateInstance();

    Context context;
};