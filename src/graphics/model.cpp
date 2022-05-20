#include "model.h"

#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtx/transform.hpp"
#include "glm/gtx/quaternion.hpp"
#include "glm/gtc/type_ptr.hpp"

#include "vulkan/context.h"
#include "vulkan/image.h"
#include "vulkan/pipeline.h"

using namespace nlohmann;

Model::Model(Context* renderContext, const std::string& path, glm::mat4 globalTransform) {
    this->context.renderContext = renderContext;
    this->context.filePath = path;

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
        this->nodes.push_back(std::make_unique<Node>(&context, nullptr, data, data["nodes"][nodeIndex]));
    }

    context.vertexBuffer.Init(context.renderContext, context.vertices, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    context.indexBuffer.Init(context.renderContext, context.indices, VK_BUFFER_USAGE_INDEX_BUFFER_BIT);

    INFO("Loaded model");
}

void Model::Render(VkCommandBuffer buffer) {
    VkDeviceSize offsets[] = { 0 };
    vkCmdBindVertexBuffers(buffer, 0, 1, &context.vertexBuffer.buffer, offsets);
    vkCmdBindIndexBuffer(buffer, context.indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);
    for (auto& node : this->nodes) {
        node->Render(buffer);
    }
}

Model::~Model() {
    context.vertexBuffer.Destroy();
    context.indexBuffer.Destroy();
}

Node::Node(ModelContext* context, Node* parent, json data, json node) : parent(parent), context(context) {
    if (node.contains("name")) {
        this->name = node["name"];
        INFO("Loading node: {}", this->name);
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

void Node::Render(VkCommandBuffer buffer) {
    for (auto& geometry : this->geometries) {
        geometry->Render(buffer);
    }

    for (auto& child : this->children) {
        child->Render(buffer);
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

uint32_t sizeOfType(const std::string& type) {
    if (type == "SCALAR") {
        return 1;
    }
    else if (type == "VEC2") {
        return 2;
    }
    else if (type == "VEC3") {
        return 3;
    }
    else if (type == "VEC4") {
        return 4;
    }
    else if (type == "MAT2") {
        return 4;
    }
    else if (type == "MAT3") {
        return 9;
    }
    else if (type == "MAT4") {
        return 16;
    }

    CRITICAL("Invalid type: {}", type);
}

std::vector<char> readAccessorData(std::filesystem::path path, json data, json accessor, uint32_t& count, uint32_t& size) {
    if (!accessor.contains("bufferView")) {
        CRITICAL("An accessor doesn't have a buffer view");
    }
    uint32_t bufferViewIndex = accessor["bufferView"];
    json bufferView = data["bufferViews"][bufferViewIndex];

    // Offset calculation
    uint32_t bufferViewOffset = 0;
    uint32_t bufferOffset = 0;
    if (accessor.contains("byteOffset")) {
        bufferViewOffset = accessor["byteOffset"];
    }
    if (bufferView["byteOffset"] != 0) {
        bufferOffset = bufferView["byteOffset"];
    }
    uint32_t offset = bufferViewOffset + bufferOffset;

    uint32_t bufferIndex = bufferView["buffer"];
    json buffer = data["buffers"][bufferIndex];

    count = accessor["count"];
    size = sizeOfComponentType(accessor["componentType"]) * sizeOfType(accessor["type"]) * count;

    // Open file and read data
    if (!buffer.contains("uri")) {
        CRITICAL("Buffer doesn't have a uri");
    }
    std::string uri = buffer["uri"];
    std::ifstream file(path.replace_filename(uri), std::ios::binary);
    if (!file.is_open()) {
        CRITICAL("Couldn't open file {}", "models/" + uri);
    }

    file.seekg(offset, std::ios::beg);
    std::vector<char> bytes(size);
    file.read(bytes.data(), bytes.size());
    file.close();

    return bytes;
}

Geometry::Geometry(ModelContext* context, Node* parent, nlohmann::json& data, nlohmann::json& primitive) : context(context) {
    glm::vec3 color = { 1.0f, 1.0f, 1.0f };
    if (primitive.contains("material")) {
        uint32_t materialIndex = primitive["material"];
        json material = data["materials"][materialIndex];
        if (material.contains("pbrMetallicRoughness")) {
            json pbr = material["pbrMetallicRoughness"];
            if (pbr.contains("baseColorFactor")) {
                std::vector<float> baseColorFactor = pbr["baseColorFactor"];
                color = { baseColorFactor[0], baseColorFactor[1], baseColorFactor[2] };
            }
        }
    }

    json attributes = primitive["attributes"];
    if (attributes.contains("POSITION")) {
        uint32_t vertexAccessorIndex = attributes["POSITION"];
        json vertexAccessor = data["accessors"][vertexAccessorIndex];

        uint32_t normalCount, normalSize;
        std::vector<char> normalBuffer;

        if (attributes.contains("NORMAL")) {
            uint32_t normalAccessorIndex = attributes["NORMAL"];
            json normalAccessor = data["accessors"][normalAccessorIndex];
            normalBuffer = readAccessorData(context->filePath, data, normalAccessor, normalCount, normalSize);
        }

        std::vector<char> uvBuffer;

        uint32_t count, size;
        std::vector<char> buffer = readAccessorData(context->filePath, data, vertexAccessor, count, size);

        this->mesh.vertices.offset = context->vertices.size();
        this->mesh.vertices.buffer = &context->vertexBuffer;
        this->mesh.vertices.size = count;
        context->vertices.resize(context->vertices.size() + count);

        for (uint32_t i = 0; i < count; i++) {
            glm::vec3 position;
            position.x = *(float*)(buffer.data() + i * 12 + 0);
            position.y = *(float*)(buffer.data() + i * 12 + 4);
            position.z = *(float*)(buffer.data() + i * 12 + 8);

            context->vertices[i] = { position, color };
        }
    }

    if (primitive.contains("indices")) {
        uint32_t indexAccessorIndex = primitive["indices"];
        json indexAccessor = data["accessors"][indexAccessorIndex];

        uint32_t count, size;
        std::vector<char> buffer = readAccessorData(context->filePath, data, indexAccessor, count, size);

        this->mesh.indices.offset = context->indices.size();
        this->mesh.indices.buffer = &context->indexBuffer;
        context->indices.resize(context->indices.size() + count);

        for (uint32_t i = 0; i < count; i++) {
            if (sizeOfComponentType(indexAccessor["componentType"]) == 1) {
                context->indices[i] = *(uint8_t*)(buffer.data() + i * 1);
                this->mesh.indices.size = size;
            }
            else if (sizeOfComponentType(indexAccessor["componentType"]) == 2) {
                context->indices[i] = *(uint16_t*)(buffer.data() + i * 2);
                this->mesh.indices.size = size / 2;
            }
            else if (sizeOfComponentType(indexAccessor["componentType"]) == 4) {
                context->indices[i] = *(uint32_t*)(buffer.data() + i * 4);
                this->mesh.indices.size = size / 4;
            }
        }

        
    }
}

Geometry::~Geometry() {}

void Geometry::Render(VkCommandBuffer buffer) const {
    VkDeviceSize offsets[] = { 0 };
    vkCmdDrawIndexed(buffer, this->mesh.indices.size, 1, this->mesh.indices.offset, this->mesh.vertices.offset, 0);
}
