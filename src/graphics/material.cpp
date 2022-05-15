#include "material.h"

#include "spirv_reflect.h"

Material::Material(std::shared_ptr<Shader> shader) : shader(shader) 
{

}