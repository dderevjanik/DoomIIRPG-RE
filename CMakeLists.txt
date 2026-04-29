cmake_minimum_required(VERSION 3.22)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/Modules")

# Require macOS 13.3+ for C++20 std::format (floating-point to_chars)
if(APPLE AND NOT CMAKE_OSX_DEPLOYMENT_TARGET)
	set(CMAKE_OSX_DEPLOYMENT_TARGET "13.3" CACHE STRING "Minimum macOS version" FORCE)
endif()

project(DRPGEngine VERSION 0.1.0)

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

# Vorbis (libvorbisfile) for streaming Ogg-Vorbis music tracks. Loaded via
# vcpkg; both libvorbis and libvorbisfile come as a single port. Falls back
# to pkg-config (e.g. Homebrew on macOS) if the vcpkg config isn't present.
find_package(Vorbis CONFIG)
if(NOT Vorbis_FOUND)
	find_package(PkgConfig QUIET)
	if(PkgConfig_FOUND)
		# IMPORTED_TARGET creates `PkgConfig::vorbisfile` with all link/include
		# flags wired correctly, including the -L path that bare _LIBRARIES misses.
		pkg_check_modules(VORBISFILE QUIET IMPORTED_TARGET vorbisfile)
	endif()
endif()

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

# Dear ImGui (developer console overlay)
FetchContent_Declare(
	imgui
	GIT_REPOSITORY https://github.com/ocornut/imgui.git
	GIT_TAG v1.91.8
	GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(imgui)

# Build ImGui as a static library with SDL2 + OpenGL2 backends
add_library(imgui STATIC
	${imgui_SOURCE_DIR}/imgui.cpp
	${imgui_SOURCE_DIR}/imgui_demo.cpp
	${imgui_SOURCE_DIR}/imgui_draw.cpp
	${imgui_SOURCE_DIR}/imgui_tables.cpp
	${imgui_SOURCE_DIR}/imgui_widgets.cpp
	${imgui_SOURCE_DIR}/backends/imgui_impl_sdl2.cpp
	${imgui_SOURCE_DIR}/backends/imgui_impl_opengl2.cpp
)
target_include_directories(imgui PUBLIC
	${imgui_SOURCE_DIR}
	${imgui_SOURCE_DIR}/backends
)
target_link_libraries(imgui PUBLIC SDL2::SDL2 OpenGL::GL)

# Global identifiers for each project/target
set(HASH_LIBRARY_TGT_NAME           Hash-Library)
add_subdirectory("${PROJECT_SOURCE_DIR}/src")
add_subdirectory("${PROJECT_SOURCE_DIR}/third_party_libs/hash-library")

# Tests
enable_testing()
add_subdirectory("${PROJECT_SOURCE_DIR}/test")