#include "model.h"

#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtx/transform.hpp"
#include "glm/gtx/quaternion.hpp"
#include "glm/gtc/type_ptr.hpp"

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
        std::vector<float> matrix = node["matrix"];
        if (matrix.size() != 16) {
            CRITICAL("Invalid matrix size on node");
        }
        this->matrix = glm::make_mat4(matrix.data());
    }

    if (node.contains("rotation") && node.contains("scale") && node.contains("translation")) {
        std::vector<float> rawTranslation = node["translation"];
        if (rawTranslation.size() != 3) {
            CRITICAL("Invalid translation");
        }
        glm::mat4 translation = glm::translate(glm::mat4(), glm::make_vec3(rawTranslation.data()));

        std::vector<float> rawScale = node["scale"];
        if (rawScale.size() != 3) {
            CRITICAL("Invalid scale");
        }
        glm::mat4 scale = glm::scale(glm::make_vec3(rawScale.data()));

        std::vector<float> rawRotation = node["rotation"];
        if (rawRotation.size() != 4) {
            CRITICAL("Invalid rotation");
        }
        glm::mat4 rotation = glm::toMat4(glm::make_quat(rawRotation.data()));
        this->matrix = translation * rotation * scale;
    }

    if (this->parent != nullptr) {
        this->matrix *= this->parent->matrix; // Multiply by parent's model matrix to get global transform
    }

    if (node.contains("mesh")) {
        uint32_t meshIndex = node["mesh"];
        json mesh = data["meshes"][meshIndex];

        if (node.contains("name")) {
            this->meshName = mesh["name"];
            INFO("Loading mesh: {}", this->meshName);
        }

        std::vector<json> primitives = mesh["primitives"];
        for (auto& primitive : primitives) {
            Geometry geometry(this, data, primitive);
            this->geometries.push_back(geometry);
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

Geometry::Geometry(Node* parent, nlohmann::json &data, nlohmann::json &primitive) {
    json attributes = primitive["attributes"];
    if (attributes.contains("POSITION")) {
        uint32_t vertexAccessorIndex = attributes["POSITION"];
        json vertexAccessor = data["accessors"][vertexAccessorIndex];

        uint32_t byteOffset = 0;
        if (vertexAccessor.contains("byteOffset")) {
            byteOffset = vertexAccessor["byteOffset"];
        }

        if (!vertexAccessor.contains("bufferView")) {
            CRITICAL("An accessor doesn't have a buffer view");
        }
        uint32_t bufferView = vertexAccessor["bufferView"];
    }
}

