#pragma once

#include "vulkan/context.h"
#include "window.h"
#include "mesh.h"
#include "material.h"
#include "vulkan/image.h"

class Renderer : public Layer {
public:
    Renderer(Window* window);
    ~Renderer();

    void Submit(const Mesh& mesh, const Material& material);

    virtual void OnTick() override;
private:
    Context context;
    Buffer<Vertex> vertexBuffer;
    Buffer<uint32_t> indexBuffer;

    Pipeline* scenePipeline{};
    Image* sceneImage = nullptr;
    VkDescriptorSet sceneTexture{};
};