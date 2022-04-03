#include "renderer.h"

#include "core/core.h"

Renderer::Renderer(Window *window) : context(window) {}

Renderer::~Renderer() {}

void Renderer::OnTick() {
  // this->context.DrawFrame();
}