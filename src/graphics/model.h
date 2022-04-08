#pragma once

#include "core/core.h"
#include "vulkan/vertex.h"
#include "vulkan/index.h"

#include "nlohmann/json.hpp"

struct Node {
    Node* parent;
    std::vector<Node> children;
    glm::mat4 matrix;
    std::string name;
    
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