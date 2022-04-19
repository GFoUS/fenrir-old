cmake_minimum_required(VERSION 3.10)

project(fenrir)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(vendor/spdlog)
add_subdirectory(vendor/glfw)
add_subdirectory(vendor/json)

find_package(Vulkan REQUIRED)

file(GLOB SRC
    "src/**/**/*.cpp"
    "src/**/*.cpp"
    "src/*.cpp"
)

include_directories(src)
include_directories(vendor/glm)
include_directories(vendor/VulkanMemoryAllocator/include)
include_directories(vendor/stb)

add_executable(fenrir ${SRC})
target_link_libraries(fenrir PRIVATE spdlog::spdlog glfw vulkan nlohmann_json::nlohmann_json)