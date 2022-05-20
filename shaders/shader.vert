#version 450

/*
layout(set = 0, binding = 0) uniform Model {
    mat4 model;
} model;
layout(set = 1, binding = 0) uniform ViewProjection {
    mat4 view;
    mat4 proj;
} vp;
*/

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
//layout(location = 2) in vec3 inNormal;
//layout(location = 3) in vec2 inUV;

//layout(location = 0) out vec3 fragColor;
//layout(location = 1) out vec3 fragPos;
//layout(location = 2) out vec3 fragNormal;
//layout(location = 3) out vec2 fragUV;

void main() {
    gl_Position = vec4(inPosition, 1.0);
    //fragColor = inColor;
    //fragPos = vec3(model.model * vec4(inPosition, 1.0));
    //fragNormal = mat3(transpose(inverse(model.model))) * inNormal;
    //fragUV = inUV;
}