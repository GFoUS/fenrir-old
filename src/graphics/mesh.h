#pragma once

#include "vulkan/buffer.h"
#include "vulkan/vertex.h"

struct Mesh {
	Mesh(Buffer<Vertex>::Ref vertices, Buffer<uint32_t>::Ref indices);
	~Mesh();
	
	Buffer<Vertex>::Ref vertices;
	Buffer<uint32_t>::Ref indices;
};