#include "iostream"

#include "spdlog/spdlog.h"
#include "core/application.h"
#include "graphics/vulkan/utils.h"

int main() {
    spdlog::set_level(spdlog::level::debug);

    INFO("{}", Pad(256, 256));
    INFO("{}", Pad(1000, 256));

    Application app;
    app.Run();


    return 0;
}