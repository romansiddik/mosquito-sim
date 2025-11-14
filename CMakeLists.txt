cmake_minimum_required(VERSION 3.10)
project(DengueSimulation)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Include vcpkg toolchain for seamless dependency management
include($ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake OPTIONAL)

find_package(glfw3 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(Threads REQUIRED)
find_package(OpenGL REQUIRED)

add_executable(dengue_sim main.cpp drawing.cpp mosquito.cpp ui.cpp)

# Include the directory containing header files
target_include_directories(dengue_sim PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# Link against the vcpkg-provided CMake targets
target_link_libraries(dengue_sim PRIVATE
    glfw
    glm::glm
    Threads::Threads
    OpenGL::GL
)

if(CMAKE_COMPILER_IS_GNUCXX AND CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 5)
    target_compile_definitions(dengue_sim PRIVATE _GLIBCXX_USE_CXX11_ABI=0)
endif()
