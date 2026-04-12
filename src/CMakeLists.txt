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
list(FILTER LIB_SOURCE_FILES EXCLUDE REGEX "engine/core/Main\\.cpp$")

add_library(drpg-engine-lib STATIC ${HEADER_FILES} ${LIB_SOURCE_FILES} ${RESOURCES_FILES})

set_target_properties(drpg-engine-lib PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

target_include_directories(drpg-engine-lib PUBLIC
	# engine root + subfolders
	${CMAKE_CURRENT_SOURCE_DIR}/engine
	${CMAKE_CURRENT_SOURCE_DIR}/engine/canvas
	${CMAKE_CURRENT_SOURCE_DIR}/engine/render
	${CMAKE_CURRENT_SOURCE_DIR}/engine/menu
	${CMAKE_CURRENT_SOURCE_DIR}/engine/graphics
	${CMAKE_CURRENT_SOURCE_DIR}/engine/audio
	${CMAKE_CURRENT_SOURCE_DIR}/engine/input
	${CMAKE_CURRENT_SOURCE_DIR}/engine/ui
	${CMAKE_CURRENT_SOURCE_DIR}/engine/core
	${CMAKE_CURRENT_SOURCE_DIR}/engine/resource
	${CMAKE_CURRENT_SOURCE_DIR}/engine/script
	${CMAKE_CURRENT_SOURCE_DIR}/engine/data
	# game root + subfolders
	${CMAKE_CURRENT_SOURCE_DIR}/game
	${CMAKE_CURRENT_SOURCE_DIR}/game/core
	${CMAKE_CURRENT_SOURCE_DIR}/game/entity
	${CMAKE_CURRENT_SOURCE_DIR}/game/combat
	${CMAKE_CURRENT_SOURCE_DIR}/game/player
	${CMAKE_CURRENT_SOURCE_DIR}/game/script
	${CMAKE_CURRENT_SOURCE_DIR}/game/minigame
	${CMAKE_CURRENT_SOURCE_DIR}/game/data
	${CMAKE_CURRENT_SOURCE_DIR}/game/ui
	${CMAKE_CURRENT_SOURCE_DIR}/game/asset
	# third-party
	${CMAKE_SOURCE_DIR}/third_party_libs/stb
)

target_link_libraries(drpg-engine-lib PUBLIC
	SDL2::SDL2
	OpenAL::OpenAL
	OpenGL::GL
	${HASH_LIBRARY_TGT_NAME}
	yaml-cpp::yaml-cpp)

# Main game executable
add_executable(${PROJECT_NAME} engine/core/Main.cpp)

set_target_properties(${PROJECT_NAME} PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

target_compile_definitions(${PROJECT_NAME} PRIVATE PROJECT_VERSION="${PROJECT_VERSION}")
target_link_libraries(${PROJECT_NAME} drpg-engine-lib)

# Converter tool
add_subdirectory(converter)
