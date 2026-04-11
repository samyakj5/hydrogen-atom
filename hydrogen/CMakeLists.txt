cmake_minimum_required(VERSION 3.10)
project(hydrogen)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(OpenGL REQUIRED)
find_package(glfw3 REQUIRED)
find_package(GLEW REQUIRED)
find_package(glm REQUIRED)

add_executable(hydrogen src/main.cpp src/hydrogen.cpp)


add_library(imgui
    ../third_party/imgui/imgui.cpp
    ../third_party/imgui/imgui_draw.cpp
    ../third_party/imgui/imgui_tables.cpp
    ../third_party/imgui/imgui_widgets.cpp
    ../third_party/imgui/backends/imgui_impl_glfw.cpp
    ../third_party/imgui/backends/imgui_impl_opengl3.cpp
)

target_include_directories(imgui PUBLIC
    ../third_party/imgui
    ../third_party/imgui/backends
)

target_compile_definitions(imgui PRIVATE IMGUI_IMPL_OPENGL_LOADER_GLEW)

target_link_libraries(imgui
    glfw
    "-framework OpenGL"
)

target_link_libraries(hydrogen
    imgui
    GLEW::GLEW
    glm::glm
)