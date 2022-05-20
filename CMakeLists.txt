cmake_minimum_required(VERSION 3.10)

project(fenrir)
set(CMAKE_CXX_STANDARD 20)
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
include_directories(vendor/SPIRV-Reflect)

include_directories(vendor/imgui)

add_executable(fenrir ${SRC} vendor/SPIRV-Reflect/spirv_reflect.c vendor/imgui/imgui.cpp vendor/imgui/imgui_draw.cpp vendor/imgui/imgui_demo.cpp vendor/imgui/imgui_tables.cpp vendor/imgui/imgui_widgets.cpp vendor/imgui/backends/imgui_impl_vulkan.cpp vendor/imgui/backends/imgui_impl_glfw.cpp)
target_link_libraries(fenrir PRIVATE spdlog::spdlog glfw Vulkan::Vulkan nlohmann_json::nlohmann_json)

set_target_properties(
    fenrir PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}")