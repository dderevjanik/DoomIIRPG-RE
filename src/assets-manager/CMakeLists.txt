set(ASSETS_MGR_TGT_NAME doom2rpg-assets-manager)

# Fetch Dear ImGui
include(FetchContent)
FetchContent_Declare(
	imgui_am
	GIT_REPOSITORY https://github.com/ocornut/imgui.git
	GIT_TAG v1.91.8
	GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(imgui_am)

# ImGui source files
set(IMGUI_SOURCES
	${imgui_am_SOURCE_DIR}/imgui.cpp
	${imgui_am_SOURCE_DIR}/imgui_draw.cpp
	${imgui_am_SOURCE_DIR}/imgui_tables.cpp
	${imgui_am_SOURCE_DIR}/imgui_widgets.cpp
	${imgui_am_SOURCE_DIR}/backends/imgui_impl_sdl2.cpp
	${imgui_am_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
)

add_executable(${ASSETS_MGR_TGT_NAME}
	main.cpp
	AssetBrowser.cpp
	SpriteLoader.cpp
	ImageLoader.cpp
	${CMAKE_SOURCE_DIR}/third_party_libs/stb/stb_image_impl.cpp
	${IMGUI_SOURCES}
)

set_target_properties(${ASSETS_MGR_TGT_NAME} PROPERTIES
	CXX_STANDARD 17
	CXX_STANDARD_REQUIRED ON
	CXX_EXTENSIONS OFF
)

target_include_directories(${ASSETS_MGR_TGT_NAME} PRIVATE
	${imgui_am_SOURCE_DIR}
	${imgui_am_SOURCE_DIR}/backends
	${CMAKE_SOURCE_DIR}/third_party_libs/stb
	${CMAKE_SOURCE_DIR}/src/game
)

target_link_libraries(${ASSETS_MGR_TGT_NAME}
	SDL2::SDL2
	OpenGL::GL
	yaml-cpp::yaml-cpp
)

# macOS framework for OpenGL deprecation warnings suppression
if(APPLE)
	target_compile_definitions(${ASSETS_MGR_TGT_NAME} PRIVATE GL_SILENCE_DEPRECATION)
endif()
