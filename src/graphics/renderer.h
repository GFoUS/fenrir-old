#pragma once

#include "vulkan/context.h"
#include "window.h"

class Renderer {
public:
    Renderer(Window* window);
    ~Renderer();
private:
    void CreateInstance();

    Context context;
};