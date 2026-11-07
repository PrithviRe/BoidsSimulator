cmake_minimum_required(VERSION 3.20)

project(BoidsSimulator LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(OpenGL REQUIRED)
find_package(glfw3 3.3 REQUIRED)

add_library(glad STATIC
    third_party/glad/src/glad.c
)

target_include_directories(glad PUBLIC
    third_party/glad/include
)

add_executable(BoidsSimulator
    src/main.cpp
)

target_include_directories(BoidsSimulator PRIVATE
    src
)

target_link_libraries(BoidsSimulator PRIVATE
    glad
    glfw
    OpenGL::GL
)

target_compile_definitions(BoidsSimulator PRIVATE
    SHADER_DIR="${CMAKE_CURRENT_SOURCE_DIR}/shaders"
)

target_compile_options(BoidsSimulator PRIVATE
    -Wall
    -Wextra
    -Wpedantic
    -O3
)

