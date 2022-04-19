#include "model.h"

#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtx/transform.hpp"
#include "glm/gtx/quaternion.hpp"
#include "glm/gtc/type_ptr.hpp"

#include "vulkan/context.h"
#include "vulkan/image.h"

using namespace nlohmann;

Model::Model(Context* renderContext, const std::string& path, glm::mat4 globalTransform) {
    this->context.renderContext = renderContext;
    this->context.filePath = path;
    this->context.matrixIndex = 0;
    this->context.globalTransform = globalTransform;

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

    // Get model matrices
    std::vector<glm::mat4> matrices;
    for (auto& node : this->nodes) {
        std::vector<glm::mat4> nodeMatrices = node->GetMatrices();
        matrices.insert(matrices.end(), nodeMatrices.begin(), nodeMatrices.end());
    }

    uint32_t paddedMat4Size = Pad(sizeof(glm::mat4), renderContext->physicalProperties.limits.minUniformBufferOffsetAlignment);
    std::vector<uint8_t> matricesData(paddedMat4Size * matrices.size(), 0);
    for (uint32_t i = 0; i < matrices.size(); i++) {
        memcpy(matricesData.data() + (i * paddedMat4Size), &matrices[i], sizeof(glm::mat4)); // Copy matrix data to padded offset location, dodgy pointer arithmetic
    }

    this->modelMatricesData = std::make_unique<Buffer<uint8_t>>(renderContext, matricesData, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);

    // Get texture data
    if (data.contains("textures")) {
        std::vector<json> textures = data["textures"];
        context.textures.resize(textures.size());
        for (uint32_t i = 0; i < textures.size(); i++) {
            json t = textures[i];
            if (!t.contains("source")) {
                CRITICAL("Texture missing source");
            }

            uint32_t imageIndex = t["source"];
            json image = data["images"][imageIndex];

            if (image.contains("bufferView")) {
                CRITICAL("Loading buffers from a buffer view isn't supported yet");
            }

            std::string uri = image["uri"];
            context.textures[i] = Image::LoadImage(context.renderContext, context.filePath.replace_filename(uri));
            INFO("Loaded texture {} in slot {}", uri, i);

            if (t.contains("sampler")) {
                uint32_t samplerIndex = t["sampler"];
                json s = data["samplers"][samplerIndex];

                VkFilter magFilter = VK_FILTER_NEAREST;
                VkFilter minFilter = VK_FILTER_NEAREST;

                if (s.contains("magFilter")) {
                    switch ((uint32_t)s["magFilter"]) {
                        case 9728: // 9728 = Nearest
                        case 9984: // 9984 = NearestMipmapNearest
                        case 9986: // 9986 = NearestMipmapLinear
                            magFilter = VK_FILTER_NEAREST;
                            break;
                        case 9729: // 9729 = Linear
                        case 9985: // 9985 = LinearMipmapNearest
                        case 9987: // 9987 = LinearMipmapLinear
                            magFilter = VK_FILTER_LINEAR;
                            break;
                        default:
                            CRITICAL("Unknown minFilter");
                    }
                }
                if (s.contains("minFilter")) {
                    switch ((uint32_t)s["minFilter"]) {
                        case 9728: // 9728 = Nearest
                        case 9984: // 9984 = NearestMipmapNearest
                        case 9986: // 9986 = NearestMipmapLinear
                            minFilter = VK_FILTER_NEAREST;
                            break;
                        case 9729: // 9729 = Linear
                        case 9985: // 9985 = LinearMipmapNearest
                        case 9987: // 9987 = LinearMipmapLinear
                            minFilter = VK_FILTER_LINEAR;
                            break;
                        default:
                        CRITICAL("Unknown minFilter");
                    }
                }

                VkSamplerAddressMode wrapS = VK_SAMPLER_ADDRESS_MODE_REPEAT;
                VkSamplerAddressMode wrapT = VK_SAMPLER_ADDRESS_MODE_REPEAT;

                if (s.contains("wrapS")) {
                    switch ((uint32_t)s["wrapS"]) {
                        case 33071: // 33071 = ClampToEdge
                            wrapS = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
                            break;
                        case 10497: // 10497 = Repeat
                            wrapS = VK_SAMPLER_ADDRESS_MODE_REPEAT;
                            break;
                        case 33648: // 33648 = MirroredRepeat
                            wrapS = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
                            break;
                        default:
                            CRITICAL("Unknown wrapS");
                    }
                }
                if (s.contains("wrapT")) {
                    switch ((uint32_t)s["wrapT"]) {
                        case 33071: // 33071 = ClampToEdge
                            wrapT = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
                            break;
                        case 10497: // 10497 = Repeat
                            wrapT = VK_SAMPLER_ADDRESS_MODE_REPEAT;
                            break;
                        case 33648: // 33648 = MirroredRepeat
                            wrapT = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
                            break;
                        default:
                            CRITICAL("Unknown wrapT");
                    }
                }
                context.textures[i]->GetSampler(magFilter, minFilter, wrapS, wrapT); // Initial get creates it with these parameters
            }
        }
    }

    VkDescriptorPoolSize modelMatricesSize{};
    modelMatricesSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    modelMatricesSize.descriptorCount = 1;
    VkDescriptorPoolSize textureSize{};
    textureSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    textureSize.descriptorCount = context.textures.size();
    std::array<VkDescriptorPoolSize, 2> poolSizes = {modelMatricesSize, textureSize};

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = poolSizes.size();
    poolInfo.pPoolSizes = poolSizes.data();
    poolInfo.maxSets = context.textures.size() + 1;

    VkResult poolResult = vkCreateDescriptorPool(renderContext->device, &poolInfo, nullptr, &this->descriptorPool);
    if (poolResult != VK_SUCCESS) {
        CRITICAL("Descriptor pool creation failed with error code: {}", poolResult);
    }

    VkDescriptorSetAllocateInfo matricesAllocInfo{};
    matricesAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    matricesAllocInfo.descriptorPool = this->descriptorPool;
    matricesAllocInfo.descriptorSetCount = 1;
    matricesAllocInfo.pSetLayouts = &renderContext->descriptorSetLayouts[0]; // Set to first set which is the model matrix

    VkResult matricesAllocResult = vkAllocateDescriptorSets(renderContext->device, &matricesAllocInfo, &context.matrixDescriptorSet);
    if (matricesAllocResult != VK_SUCCESS) {
        CRITICAL("Descriptor set allocation failed with error code: {}", matricesAllocResult);
    }

    std::vector<VkDescriptorSetLayout> textureLayouts(context.textures.size(), renderContext->descriptorSetLayouts[2]);
    VkDescriptorSetAllocateInfo textureAllocInfo{};
    textureAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    textureAllocInfo.descriptorPool = this->descriptorPool;
    textureAllocInfo.descriptorSetCount = context.textures.size();
    textureAllocInfo.pSetLayouts = textureLayouts.data();

    context.textureSets.resize(context.textures.size());
    VkResult textureAllocResult = vkAllocateDescriptorSets(renderContext->device, &textureAllocInfo, context.textureSets.data());
    if (textureAllocResult != VK_SUCCESS) {
        CRITICAL("Descriptor set allocation failed with error code: {}", textureAllocResult);
    }

    VkDescriptorBufferInfo bufferInfo{};
    bufferInfo.buffer = this->modelMatricesData->buffer;
    bufferInfo.offset = 0;
    bufferInfo.range = sizeof(glm::mat4);
    VkWriteDescriptorSet bufferWrite{};
    bufferWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    bufferWrite.dstSet = context.matrixDescriptorSet;
    bufferWrite.dstBinding = 0;
    bufferWrite.dstArrayElement = 0;
    bufferWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    bufferWrite.descriptorCount = 1;
    bufferWrite.pBufferInfo = &bufferInfo;

    std::vector<VkWriteDescriptorSet> descriptorWrites(context.textures.size() + 1);
    std::vector<VkDescriptorImageInfo*> imageInfos(context.textures.size());

    for (uint32_t i = 0; i < context.textures.size(); i++) {
        auto* imageInfo = new VkDescriptorImageInfo();
        imageInfo->imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo->imageView = context.textures[i]->GetImageView(VK_IMAGE_ASPECT_COLOR_BIT);
        imageInfo->sampler = context.textures[i]->GetSampler();

        VkWriteDescriptorSet textureWrite{};
        textureWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        textureWrite.dstSet = context.textureSets[i];
        textureWrite.dstBinding = 0;
        textureWrite.dstArrayElement = 0;
        textureWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        textureWrite.descriptorCount = 1;
        textureWrite.pImageInfo = imageInfo;

        descriptorWrites[i] = textureWrite;
    }

    for (auto& imageInfo : imageInfos) {
        delete imageInfo;
    }

    descriptorWrites[context.textures.size()] = bufferWrite;

    vkUpdateDescriptorSets(renderContext->device, descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);

    INFO("Loaded model");
}

void Model::Render(VkCommandBuffer buffer) {
    this->context.matrixIndex = 0;
    for (auto &node: this->nodes) {
        node->Render(buffer);
    }
}

Model::~Model() {
    vkDeviceWaitIdle(context.renderContext->device);
    vkDestroyDescriptorPool(context.renderContext->device, this->descriptorPool, nullptr);
}

Node::Node(ModelContext* context, Node* parent, json data, json node) : parent(parent), context(context) {
    if (node.contains("name")) {
        this->name = node["name"];
        INFO("Loading node: {}", this->name);
    }

    this->matrix = glm::mat4(1.0f); // Default identity matrix to pass through parents' matrix to children

    if (node.contains("matrix")) {
        std::vector<float> m = node["matrix"];
        if (m.size() != 16) {
            CRITICAL("Invalid matrix size on node");
        }
        this->matrix = glm::make_mat4(m.data());
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
        this->matrix = this->parent->matrix * this->matrix; // Multiply by parent's model matrix to get global transform
    } else {
        this->matrix = context->globalTransform * this->matrix; // If no parent, parent is the model class which will have a global transform
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
    uint32_t matrixOffset = context->matrixIndex * Pad(sizeof(glm::mat4), context->renderContext->physicalProperties.limits.minUniformBufferOffsetAlignment);
    vkCmdBindDescriptorSets(buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, context->renderContext->pipelineLayout, 0, 1, &context->matrixDescriptorSet, 1, &matrixOffset);
    context->matrixIndex++;

    for (auto& geometry : this->geometries) {
        geometry->Render(buffer);
    }

    for (auto& child : this->children) {
        child->Render(buffer);
    }
}

std::vector<glm::mat4> Node::GetMatrices() {
    std::vector<glm::mat4> matrices;
    matrices.push_back(this->matrix);
    for (auto& child : this->children) {
        std::vector<glm::mat4> childMatrices = child->GetMatrices();
        matrices.insert(matrices.end(), childMatrices.begin(), childMatrices.end());
    }
    return matrices;
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

std::vector<char> readAccessorData(std::filesystem::path path, json data, json accessor, uint32_t &count, uint32_t &size) {
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

Geometry::Geometry(ModelContext* context, Node* parent, nlohmann::json &data, nlohmann::json &primitive) : context(context) {
    glm::vec3 color = {1.0f, 1.0f, 1.0f};
    if (primitive.contains("material")) {
        uint32_t materialIndex = primitive["material"];
        json material = data["materials"][materialIndex];
        if (material.contains("pbrMetallicRoughness")) {
            json pbr = material["pbrMetallicRoughness"];
            if (pbr.contains("baseColorFactor")) {
                std::vector<float> baseColorFactor = pbr["baseColorFactor"];
                color = {baseColorFactor[0], baseColorFactor[1], baseColorFactor[2]};
            }

            if (pbr.contains("baseColorTexture")) {
                json baseColorTexture = pbr["baseColorTexture"];
                if (baseColorTexture.contains("index")) {
                    this->textureIndex = baseColorTexture["index"];
                    INFO("Using texture with index {}", this->textureIndex);
                }
                if (baseColorTexture.contains("texCoord")) {
                    this->textureCoordIndex = baseColorTexture["texCoord"];
                } else {
                    this->textureCoordIndex = 0;
                }
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

        if (attributes.contains("TEXCOORD_" + std::to_string(this->textureCoordIndex))) {
            uint32_t uvAccessorIndex = attributes["TEXCOORD_" + std::to_string(this->textureCoordIndex)];
            json uvAccessor = data["accessors"][uvAccessorIndex];
            uint32_t uvCount, uvSize;
            uvBuffer = readAccessorData(context->filePath, data, uvAccessor, uvCount, uvSize);
        }

        uint32_t count, size;
        std::vector<char> buffer = readAccessorData(context->filePath, data, vertexAccessor, count, size);

        this->vertices.resize(count);
        for (uint32_t i = 0; i < count; i++) {
            glm::vec3 position;
            position.x = *(float*)(buffer.data() + i * 12 + 0);
            position.y = *(float*)(buffer.data() + i * 12 + 4);
            position.z = *(float*)(buffer.data() + i * 12 + 8);

            glm::vec3 normal;
            if (attributes.contains("NORMAL")) {
                normal.x = *(float*)(normalBuffer.data() + i * 12 + 0);
                normal.y = *(float*)(normalBuffer.data() + i * 12 + 4);
                normal.z = *(float*)(normalBuffer.data() + i * 12 + 8);
            }

            glm::vec2 uv;
            if (attributes.contains("TEXCOORD_" + std::to_string(this->textureCoordIndex))) {
                uv.x = *(float*)(uvBuffer.data() + i * 8 + 0);
                uv.y = *(float*)(uvBuffer.data() + i * 8 + 4);
            }

            this->vertices[i] = {position, color, normal, uv};
        }

        this->vertexBuffer = std::make_unique<Buffer<Vertex>>(context->renderContext, this->vertices, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    }

    if (primitive.contains("indices")) {
        uint32_t indexAccessorIndex = primitive["indices"];
        json indexAccessor = data["accessors"][indexAccessorIndex];

        uint32_t count, size;
        std::vector<char> buffer = readAccessorData(context->filePath, data, indexAccessor, count, size);

        this->indices.resize(count);
        for (uint32_t i = 0; i < count; i++) {
            if (sizeOfComponentType(indexAccessor["componentType"]) == 1) {
                this->indices[i] = *(uint8_t*)(buffer.data() + i * 1);
            } else if (sizeOfComponentType(indexAccessor["componentType"]) == 2) {
                this->indices[i] = *(uint16_t*)(buffer.data() + i * 2);
            } else if (sizeOfComponentType(indexAccessor["componentType"]) == 4) {
                this->indices[i] = *(uint32_t*)(buffer.data() + i * 4);
            }
        }

        this->indexBuffer = std::make_unique<Buffer<uint32_t>>(context->renderContext, this->indices, VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
    }
}

Geometry::~Geometry() {
    vkDeviceWaitIdle(context->renderContext->device);
    this->vertexBuffer->Destroy();
    this->indexBuffer->Destroy();
}

void Geometry::Render(VkCommandBuffer buffer) const {
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(buffer, 0, 1, &this->vertexBuffer->buffer, offsets);
    vkCmdBindIndexBuffer(buffer, this->indexBuffer->buffer, 0, VK_INDEX_TYPE_UINT32);
    vkCmdBindDescriptorSets(buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, context->renderContext->pipelineLayout, 2, 1, &context->textureSets[this->textureIndex], 0, nullptr);
    vkCmdDrawIndexed(buffer, this->indices.size(), 1, 0, 0, 0);
}

