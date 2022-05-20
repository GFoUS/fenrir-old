#pragma once

#include "core/core.h"
#include "vulkan/buffer.h"
#include "vulkan/vertex.h"
#include "vulkan/uniform.h"
#include "vulkan/image.h"
#include "mesh.h"

#include "nlohmann/json.hpp"

struct ModelContext {
    Context* renderContext;
    std::filesystem::path filePath;
    std::vector<Vertex> vertices;
    Buffer<Vertex> vertexBuffer;
    std::vector<uint32_t> indices;
    Buffer<uint32_t> indexBuffer;
};

struct Geometry {
    Geometry(ModelContext* context, struct Node* parent, nlohmann::json& data, nlohmann::json& primitive);
    ~Geometry();

    void Render(VkCommandBuffer buffer) const;

    ModelContext* context;
    
    Mesh mesh;
};

struct Node {
    ModelContext* context;
    Node* parent;
    std::vector<std::unique_ptr<Node>> children;
    std::string name;
    std::string meshName;
    std::vector<std::unique_ptr<Geometry>> geometries;

    Node(ModelContext* context, Node* parent, nlohmann::json data, nlohmann::json node);
    void Render(VkCommandBuffer buffer);
};

struct Model {
    ModelContext context;
    std::string sceneName;
    std::vector<std::unique_ptr<Node>> nodes;

    Model(Context* context, const std::string& path, glm::mat4 globalTransform = glm::mat4(1.0f));
    ~Model();
    void Render(VkCommandBuffer buffer);
};