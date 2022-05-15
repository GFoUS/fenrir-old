#include "mesh.h"

Mesh::Mesh(Buffer<Vertex>::Ref vertices, Buffer<uint32_t>::Ref indices) : vertices(vertices), indices(indices) {}

Mesh::~Mesh() {}
