#include "context.h"
#include "instance.h"
#include "physical.h"
#include "device.h"
#include "surface.h"

Context::Context(Window* window) {
    this->window = window;
    this->instance = tk::GetInstance();
    INFO("Instance created");
    this->surface = tk::GetSurface(this->instance, this->window);
    INFO("Surface created");
    this->physical = tk::PickPhysical(this->instance);
    INFO("Physical device picked");
    this->device = tk::GetDevice(this->physical, this->surface);
    INFO("Logical device created");
    this->graphicsQueue = tk::GetGraphicsQueue(this->physical, this->device);
    INFO("Got graphics queue");
    this->presentQueue = tk::GetPresentQueue(this->physical, this->device, this->surface);
    INFO("Got present queue");
}

Context::~Context() {
    vkDestroySurfaceKHR(this->instance, this->surface, nullptr);
    vkDestroyDevice(this->device, nullptr);
    vkDestroyInstance(this->instance, nullptr);
}