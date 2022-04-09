#pragma once

#include "core/core.h"
#include "vulkan/vertex.h"
#include "vulkan/index.h"

#include "nlohmann/json.hpp"

struct Geometry {
    Geometry(struct Node* parent, nlohmann::json &data, nlohmann::json &primitive);

    VertexBuffer vertexBuffer;
    IndexBuffer indexBuffer;
};

struct Node {
    Node* parent;
    std::vector<Node> children;
    glm::mat4 matrix;
    std::string name;
    std::string meshName;
    std::vector<Geometry> geometries;
    
    Node(Node* parent, nlohmann::json data, nlohmann::json node);
};

struct Model {
    VertexBuffer vertices;
    IndexBuffer indices;
    Context* context;
    std::string sceneName;
    std::vector<Node> nodes;

    Model(Context* context, std::string path);
};