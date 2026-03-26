cmake_minimum_required(VERSION 3.22)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/Modules")

project(DoomIIRPG VERSION 0.1.0)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Default to RelWithDebInfo so stack traces have symbols
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
	set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "Build type" FORCE)
	message(STATUS "Build type not set — defaulting to RelWithDebInfo")
endif()

# Optional AddressSanitizer for catching memory bugs
option(ENABLE_ASAN "Enable AddressSanitizer" OFF)
if(ENABLE_ASAN)
	add_compile_options(-fsanitize=address -fno-omit-frame-pointer)
	add_link_options(-fsanitize=address)
endif()

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