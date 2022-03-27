cmake_minimum_required(VERSION 3.10)

project(fenrir)

add_subdirectory(vendor/spdlog)
add_subdirectory(vendor/glfw)

find_package(Vulkan REQUIRED)

file(GLOB BLAST_SRC
    "src/**/**/*.cpp"
    "src/**/*.cpp"
    "src/*.cpp"
)

include_directories("src")

message("${BLAST_SRC}")

add_executable(blast ${BLAST_SRC})
target_link_libraries(blast PRIVATE spdlog::spdlog glfw vulkan)