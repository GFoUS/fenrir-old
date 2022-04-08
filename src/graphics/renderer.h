#pragma once

#include "vulkan/index.h"
#include "vulkan/context.h"
#include "window.h"
#include "model.h"

class Renderer : public Layer {
public:
    Renderer(Window* window);
    ~Renderer();

    virtual void OnTick() override;
    void BindModel();
private:
    Context context;
    VertexBuffer vertices;
    IndexBuffer indices;
    Model model;
};