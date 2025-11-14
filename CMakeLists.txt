'''
cmake_minimum_required(VERSION 3.10)
project(DengueSimulation)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(OpenGL REQUIRED)
find_package(glfw3 3.3 REQUIRED)
find_package(Threads REQUIRED)

add_executable(dengue_sim MyProject.cpp)

target_link_libraries(dengue_sim PRIVATE
    glfw
    OpenGL::GL
    Threads::Threads
)

if(CMAKE_COMPILER_IS_GNUCXX AND CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 5)
    target_compile_definitions(dengue_sim PRIVATE _GLIBCXX_USE_CXX11_ABI=0)
endif()
'''