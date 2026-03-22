set(EDITOR_TGT_NAME doom2rpg-editor)

# Fetch Dear ImGui
include(FetchContent)
FetchContent_Declare(
	imgui
	GIT_REPOSITORY https://github.com/ocornut/imgui.git
	GIT_TAG v1.91.8
	GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(imgui)

# ImGui source files (no CMakeLists.txt in imgui repo)
set(IMGUI_SOURCES
	${imgui_SOURCE_DIR}/imgui.cpp
	${imgui_SOURCE_DIR}/imgui_draw.cpp
	${imgui_SOURCE_DIR}/imgui_tables.cpp
	${imgui_SOURCE_DIR}/imgui_widgets.cpp
	${imgui_SOURCE_DIR}/backends/imgui_impl_sdl2.cpp
	${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
)

add_executable(${EDITOR_TGT_NAME}
	main.cpp
	Camera.cpp
	EditorUI.cpp
	MapDataIO.cpp
	BSPCompiler.cpp
	${IMGUI_SOURCES}
)

set_target_properties(${EDITOR_TGT_NAME} PROPERTIES
	CXX_STANDARD 17
	CXX_STANDARD_REQUIRED ON
	CXX_EXTENSIONS OFF
)

target_include_directories(${EDITOR_TGT_NAME} PRIVATE
	${imgui_SOURCE_DIR}
	${imgui_SOURCE_DIR}/backends
)

target_link_libraries(${EDITOR_TGT_NAME}
	doom2rpg-lib
)

# macOS framework for OpenGL deprecation warnings suppression
if(APPLE)
	target_compile_definitions(${EDITOR_TGT_NAME} PRIVATE GL_SILENCE_DEPRECATION)
endif()
