#version 450

layout(location = 0) in vec3 fragColor;
//layout(location = 1) in vec3 fragPos;
//layout(location = 2) in vec3 fragNormal;
//layout(location = 3) in vec2 fragUV;

layout(location = 0) out vec4 outColor;

//layout(set = 2, binding = 0) uniform sampler2D texSampler;


void main() {
    /*vec3 lightPos = vec3(1000.0, 1000.0, 1000.0);
    float ambientStrength = 0.1;
    vec3 lightColor = vec3(1.0, 1.0, 1.0);

    vec3 normal = normalize(fragNormal);
    vec3 lightDir = normalize(lightPos - fragPos);
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = diff * lightColor;

    vec3 ambient = ambientStrength * lightColor;
    outColor = vec4(texture(texSampler, fragUV).rgb * (ambient + diffuse), texture(texSampler, fragUV).a);
    */

    outColor = vec4(fragColor, 1.0f);
}