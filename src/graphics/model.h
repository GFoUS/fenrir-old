#pragma once

#include "core/core.h"
#include "vulkan/vertex.h"
#include "vulkan/index.h"

#include "nlohmann/json.hpp"

struct Geometry {
    Geometry(Context* context, struct Node* parent, nlohmann::json &data, nlohmann::json &primitive);
    ~Geometry();    

    std::vector<Vertex> vertices;
    VertexBuffer vertexBuffer;
    IndexBuffer indexBuffer;
};

struct Node {
    Node* parent;
    std::vector<std::unique_ptr<Node>> children;
    glm::mat4 matrix;
    std::string name;
    std::string meshName;
    std::vector<std::unique_ptr<Geometry>> geometries;
    
    Node(Context* context, Node* parent, nlohmann::json data, nlohmann::json node);
};

struct Model {
    VertexBuffer vertices;
    IndexBuffer indices;
    Context* context;
    std::string sceneName;
    std::vector<std::unique_ptr<Node>> nodes;

    Model(Context* context, std::string path);
};