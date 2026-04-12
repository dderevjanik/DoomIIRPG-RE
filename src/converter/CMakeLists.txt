set(CONVERTER_TGT_NAME drpg-convert)

# --- Embed YAML resource files into a C++ header ---
set(RESOURCES_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/resources")
set(RESOURCES_DIR "${RESOURCES_ROOT}/d2-rpg")
set(RESOURCES_HEADER "${CMAKE_CURRENT_BINARY_DIR}/resources_embedded.h")

file(GLOB_RECURSE RESOURCE_YAML_FILES "${RESOURCES_DIR}/*.yaml")

add_custom_command(
	OUTPUT "${RESOURCES_HEADER}"
	COMMAND ${CMAKE_COMMAND}
		-DRESOURCES_DIR=${RESOURCES_DIR}
		-DOUTPUT_FILE=${RESOURCES_HEADER}
		-P ${RESOURCES_ROOT}/embed_resources.cmake
	DEPENDS ${RESOURCE_YAML_FILES} ${RESOURCES_ROOT}/embed_resources.cmake
	COMMENT "Embedding converter resource YAML files"
)
add_custom_target(converter_resources DEPENDS "${RESOURCES_HEADER}")

add_executable(${CONVERTER_TGT_NAME}
	main.cpp
	d2-converter.cpp
	d1-converter.cpp
	BspToBin.cpp
	stb_image_write_impl.cpp
	Error.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/../engine/resource/ZipFile.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/../editor/MapDataIO.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/../editor/BSPCompiler.cpp
)

add_dependencies(${CONVERTER_TGT_NAME} converter_resources)

set_target_properties(${CONVERTER_TGT_NAME} PROPERTIES
	CXX_STANDARD 20
	CXX_STANDARD_REQUIRED ON
	CXX_EXTENSIONS OFF
)

target_include_directories(${CONVERTER_TGT_NAME} PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}
	${CMAKE_CURRENT_SOURCE_DIR}/../engine
	${CMAKE_CURRENT_SOURCE_DIR}/../engine/core
	${CMAKE_CURRENT_SOURCE_DIR}/../engine/resource
	${CMAKE_CURRENT_SOURCE_DIR}/../engine/data
	${CMAKE_CURRENT_SOURCE_DIR}/../game
	${CMAKE_CURRENT_SOURCE_DIR}/../editor
	${CMAKE_CURRENT_BINARY_DIR}
	${CMAKE_SOURCE_DIR}/third_party_libs/stb
)

target_link_libraries(${CONVERTER_TGT_NAME}
	SDL2::SDL2
	ZLIB::ZLIB
	yaml-cpp::yaml-cpp
)
