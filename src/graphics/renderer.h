#pragma once

#include "vulkan/context.h"

class Renderer {
public:
    Renderer();
    ~Renderer();
private:
    void CreateInstance();

    Context context;
};