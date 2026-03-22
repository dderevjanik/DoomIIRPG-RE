cmake_minimum_required(VERSION 3.22)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/Modules")

project(DoomIIRPG VERSION 0.1.0)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_compile_options(
    -fsigned-char         # Force char to be signed
)

if(MSVC)
	add_definitions("/D_CRT_SECURE_NO_WARNINGS")
endif()

if(UNIX)
	add_definitions("-Wno-write-strings")
endif()

find_package(SDL2 REQUIRED)
find_package(ZLIB REQUIRED)
find_package(OpenAL REQUIRED)
find_package(OpenGL REQUIRED)

# yaml-cpp (fetched at configure time)
include(FetchContent)
FetchContent_Declare(
	yaml-cpp
	GIT_REPOSITORY https://github.com/jbeder/yaml-cpp.git
	GIT_TAG 0.8.0
	GIT_SHALLOW TRUE
	SYSTEM
)
set(CMAKE_POLICY_VERSION_MINIMUM 3.5 CACHE STRING "" FORCE)
set(YAML_CPP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(YAML_CPP_BUILD_TOOLS OFF CACHE BOOL "" FORCE)
set(YAML_CPP_BUILD_CONTRIB OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(yaml-cpp)

# Global identifiers for each project/target
set(HASH_LIBRARY_TGT_NAME           Hash-Library)
add_subdirectory("${PROJECT_SOURCE_DIR}/src")
add_subdirectory("${PROJECT_SOURCE_DIR}/third_party_libs/hash-library")

# Tests
enable_testing()
add_subdirectory("${PROJECT_SOURCE_DIR}/test")