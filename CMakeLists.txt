cmake_minimum_required(VERSION 3.16)
project(FireworksParticleSystem3D LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

# CMake 4.x compatibility for dependencies with older policy declarations.
if (CMAKE_VERSION VERSION_GREATER_EQUAL "4.0")
  set(CMAKE_POLICY_VERSION_MINIMUM 3.5)
endif()

# GLFW
FetchContent_Declare(
  glfw
  GIT_REPOSITORY https://github.com/glfw/glfw.git
  GIT_TAG 3.3.9
)
FetchContent_MakeAvailable(glfw)

# GLM
FetchContent_Declare(
  glm
  GIT_REPOSITORY https://github.com/g-truc/glm.git
  GIT_TAG 1.0.1
)
FetchContent_MakeAvailable(glm)

# GLAD
FetchContent_Declare(
  glad
  GIT_REPOSITORY https://github.com/Dav1dde/glad.git
  GIT_TAG v0.1.36
)
FetchContent_MakeAvailable(glad)

add_executable(fireworks
  src/main.cpp
)

target_include_directories(fireworks PRIVATE
  ${glad_SOURCE_DIR}/include
)

target_link_libraries(fireworks PRIVATE glfw glad glm)
if (WIN32)
  target_link_libraries(fireworks PRIVATE winmm)
endif()

if (MSVC)
  target_compile_options(fireworks PRIVATE /W4)
else()
  target_compile_options(fireworks PRIVATE -Wall -Wextra -pedantic)
endif()
