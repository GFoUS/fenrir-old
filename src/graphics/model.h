#pragma once

#include "core/core.h"
#include "vulkan/buffer.h"
#include "vulkan/vertex.h"
#include "vulkan/uniform.h"

#include "nlohmann/json.hpp"

struct ModelContext {
    Context* renderContext;
    std::filesystem::path filePath;

    uint32_t matrixIndex;
    VkDescriptorSet matrixDescriptorSet;
    glm::mat4 globalTransform;
};

struct Geometry {
    Geometry(ModelContext* context, struct Node* parent, nlohmann::json &data, nlohmann::json &primitive);
    ~Geometry();

    void Render(VkCommandBuffer buffer) const;

    ModelContext* context;
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    std::unique_ptr<Buffer<Vertex>> vertexBuffer;
    std::unique_ptr<Buffer<uint32_t>> indexBuffer;
};

struct Node {
    ModelContext* context;
    Node* parent;
    std::vector<std::unique_ptr<Node>> children;
    glm::mat4 matrix;
    std::string name;
    std::string meshName;
    std::vector<std::unique_ptr<Geometry>> geometries;

    Node(ModelContext* context, Node* parent, nlohmann::json data, nlohmann::json node);
    void Render(VkCommandBuffer buffer);
    std::vector<glm::mat4> GetMatrices();
};

struct Model {
    ModelContext context;
    std::string sceneName;
    std::vector<std::unique_ptr<Node>> nodes;
    std::unique_ptr<Buffer<uint8_t>> modelMatricesData;

    VkDescriptorPool descriptorPool;

    Model(Context* context, const std::string& path, glm::mat4 globalTransform = glm::mat4(1.0f));
    ~Model();
    void Render(VkCommandBuffer buffer);
};