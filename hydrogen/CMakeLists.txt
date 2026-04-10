cmake_minimum_required(VERSION 3.10)
project(hydrogen)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(OpenGL REQUIRED)
find_package(glfw3 REQUIRED)
find_package(GLEW REQUIRED)

add_executable(hydrogen src/main.cpp src/hydrogen.cpp)

target_link_libraries(hydrogen
    "-framework OpenGL"
    glfw
    GLEW::GLEW
)