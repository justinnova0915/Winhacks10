cmake_minimum_required(VERSION 4.2)

set(libs "${CMAKE_SOURCE_DIR}/libs")
set(src "${CMAKE_SOURCE_DIR}/src")
set(CMakeUtils "${libs}/CMake")

include("${CMakeUtils}/optimization.cmake")

project(WinHacks)

# dependencies
find_package(glfw3 CONFIG REQUIRED)
find_package(OpenGL REQUIRED)


add_executable(WinHacks "${src}/main.cpp" "${src}/Project.hpp")
target_include_directories(WinHacks PRIVATE 
	"${CMAKE_SOURCE_DIR}"
	"${libs}"
	"${src}"
)

# project settings
target_compile_features(WinHacks PUBLIC cxx_std_20)

# dependencies
target_link_libraries(WinHacks PRIVATE 
	glfw
	OpenGL::GL
)


# optimization
add_release_ops(WinHacks)

# build
set_target_properties(WinHacks PROPERTIES
	RUNTIME_OUTPUT_DIRECTORY_DEBUG   "${CMAKE_BINARY_DIR}/bin"
	RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/bin"
)

install(TARGETS WinHacks
    RUNTIME DESTINATION "bin"
)