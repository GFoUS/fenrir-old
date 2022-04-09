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
        this->nodes.push_back(std::make_unique<Node>(context, nullptr, data, data["nodes"][nodeIndex]));
    }
}

Node::Node(Context* context, Node* parent, json data, json node) : parent(parent) {
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
            INFO("Creating geometry");
            this->geometries.push_back(std::make_unique<Geometry>(context, this, data, primitive));
        }
    }

    if (node.contains("children")) {
        std::vector<uint32_t> childrenIndices = node["children"];
        for (const auto childrenIndex : childrenIndices) {
            this->children.push_back(std::make_unique<Node>(context, this, data, data["nodes"][childrenIndex]));
        }
    }    
}

uint32_t sizeOfComponentType(uint32_t componentType) {
    switch (componentType) {
        case 5120: return 1;
        case 5121: return 1;
        case 5122: return 2;
        case 5123: return 2;
        case 5125: return 4;
        case 5126: return 4;
    }
    
    CRITICAL("Invalid component type: {}", componentType);
}

uint32_t sizeOfType(std::string type) {
    if (type == "SCALAR") {
        return 1;
    } else if (type == "VEC2") {
        return 2;
    } else if (type == "VEC3") {
        return 3;
    } else if (type == "VEC4") {
        return 4;
    } else if (type == "MAT2") {
        return 4;
    } else if (type == "MAT3") {
        return 9;
    } else if (type == "MAT4") {
        return 16;
    }

    CRITICAL("Invalid type: {}", type);
}

Geometry::Geometry(Context* context, Node* parent, nlohmann::json &data, nlohmann::json &primitive) {
    json attributes = primitive["attributes"];
    if (attributes.contains("POSITION")) {
        uint32_t vertexAccessorIndex = attributes["POSITION"];
        json vertexAccessor = data["accessors"][vertexAccessorIndex];

        // Verify accessor is for positions
        if (vertexAccessor["componentType"] != 5126) {
            CRITICAL("Only floats are supported as vertex inputs");
        }
        if (vertexAccessor["type"] != "VEC3") {
            CRITICAL("Only 3 value vectors are supported as vertex inputs");
        }

        if (!vertexAccessor.contains("bufferView")) {
            CRITICAL("An accessor doesn't have a buffer view");
        }
        uint32_t vertexBufferViewIndex = vertexAccessor["bufferView"];
        json vertexBufferView = data["bufferViews"][vertexBufferViewIndex];

        // Offset calculation
        uint32_t bufferViewOffset = 0;
        uint32_t bufferOffset = 0;
        if (vertexAccessor.contains("byteOffset")) {
            bufferViewOffset = vertexAccessor["byteOffset"];
        }
        if (vertexBufferView["byteOffset"] != 0) {
            bufferOffset = vertexBufferView["byteOffset"];
        }
        uint32_t offset = bufferViewOffset + bufferOffset;

        uint32_t length = vertexBufferView["byteLength"];
        uint32_t vertexBufferIndex = vertexBufferView["buffer"];
        json vertexBuffer = data["buffers"][vertexBufferIndex];

        uint32_t count = vertexAccessor["count"];
        uint32_t size = sizeOfComponentType(vertexAccessor["componentType"]) * sizeOfType(vertexAccessor["type"]) * count;

        // Open file and read data
        if (!vertexBuffer.contains("uri")) {
            CRITICAL("Buffer doesn't have a uri");
        }
        std::string uri = vertexBuffer["uri"];
        std::ifstream file("models/" + uri, std::ios::binary);
        if (!file.is_open()) {
            CRITICAL("Couldn't open file {}", "models/" + uri);
        }

        file.seekg(offset, std::ios::beg);
        std::vector<char> buffer(size);
        file.read(buffer.data(), buffer.size());
        file.close();

        this->vertices.resize(count);
        for (uint32_t i = 0; i < count; i++) {
            glm::vec3 position;
            position.x = *(float*)(buffer.data() + i * 12 + 0);
            position.y = *(float*)(buffer.data() + i * 12 + 4);
            position.z = *(float*)(buffer.data() + i * 12 + 8);
            this->vertices[i] = {position, glm::vec3(1.0f, 1.0f, 1.0f)};
        }

        this->vertexBuffer = VertexBuffer(context, this->vertices);
        INFO("Loaded vertex buffer");
    }
}

Geometry::~Geometry() {
    this->vertexBuffer.Destroy();
}

