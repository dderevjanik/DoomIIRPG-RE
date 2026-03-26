file(GLOB_RECURSE HEADER_FILES
	"engine/*.hpp" "engine/*.h"
	"game/*.hpp" "game/*.h")

file(GLOB_RECURSE SOURCE_FILES
	"engine/*.cpp" "engine/*.c"
	"game/*.cpp" "game/*.c")

# ZipFile is only used by the converter tool, not the engine
list(FILTER SOURCE_FILES EXCLUDE REGEX "ZipFile\\.cpp$")
list(FILTER HEADER_FILES EXCLUDE REGEX "ZipFile\\.h$")

file(GLOB_RECURSE RESOURCES_FILES
	"engine/*.rc" "engine/*.ico"
	"game/*.rc" "game/*.ico")

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} PREFIX "Header Files" FILES ${HEADER_FILES})
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} PREFIX "Source Files" FILES ${SOURCE_FILES})
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} PREFIX "Resources Files" FILES ${RESOURCES_FILES})

# Engine+game as static library (exclude Main.cpp — each target has its own)
set(LIB_SOURCE_FILES ${SOURCE_FILES})
list(FILTER LIB_SOURCE_FILES EXCLUDE REGEX "engine/Main\\.cpp$")

add_library(doom2rpg-lib STATIC ${HEADER_FILES} ${LIB_SOURCE_FILES} ${RESOURCES_FILES})

set_target_properties(doom2rpg-lib PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

target_include_directories(doom2rpg-lib PUBLIC
	${CMAKE_CURRENT_SOURCE_DIR}/engine
	${CMAKE_CURRENT_SOURCE_DIR}/game
)

target_link_libraries(doom2rpg-lib PUBLIC
	SDL2::SDL2
	OpenAL::OpenAL
	OpenGL::GL
	${HASH_LIBRARY_TGT_NAME}
	yaml-cpp::yaml-cpp)

# Main game executable
add_executable(${PROJECT_NAME} engine/Main.cpp)

set_target_properties(${PROJECT_NAME} PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

target_link_libraries(${PROJECT_NAME} doom2rpg-lib)

# Converter tool
add_subdirectory(converter)

# Map editor
add_subdirectory(editor)

# Assets manager
add_subdirectory(assets-manager)
