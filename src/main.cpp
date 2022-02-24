#include "iostream"

#include "spdlog/spdlog.h"
#include "core/application.h"

int main() {
    spdlog::set_level(spdlog::level::debug);

    Application app;
    app.Run();

    return 0;
}