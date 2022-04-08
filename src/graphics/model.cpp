#include "model.h"

using namespace nlohmann;

Model::Model(Context* context, std::string path) : context(context) {
    std::ifstream file(path);
    if (!file.is_open()) {
        CRITICAL("Couldn't open file {}", path);
    }

    json data;
    file >> data;
    uint32_t sceneNumber = data["scene"];
    json scene = data["scenes"][sceneNumber];

    if (scene.contains("name")) {
        this->sceneName = scene["name"];
        INFO("Loading scene: {}", this->sceneName);
    }
    
    std::vector<uint32_t> nodeIndices = scene["nodes"];
    for (const auto nodeIndex : nodeIndices) {
        Node node(nullptr, data, data["nodes"][nodeIndex]);
        this->nodes.push_back(node);
    }
}

Node::Node(Node* parent, json data, json node) : parent(parent) {
    if (node.contains("name")) {
        this->name = node["name"];
        INFO("Loading node: {}", this->name);
    }

    if (node.contains("matrix")) {
        std::vector<uint32_t> matrix = node["matrix"];
        if (matrix.size() != 16) {
            CRITICAL("Invalid matrix size on node");
        }
    }

    if (node.contains("children")) {
        std::vector<uint32_t> childrenIndices = node["children"];
        for (const auto childrenIndex : childrenIndices) {
            Node child(this, data, data["nodes"][childrenIndex]);
            this->children.push_back(child);
        }
    }    
}
