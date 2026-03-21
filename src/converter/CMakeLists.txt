set(CONVERTER_TGT_NAME doom2rpg-convert)

add_executable(${CONVERTER_TGT_NAME}
	main.cpp
	Error.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/../engine/ZipFile.cpp
)

set_target_properties(${CONVERTER_TGT_NAME} PROPERTIES
	CXX_STANDARD 17
	CXX_STANDARD_REQUIRED ON
	CXX_EXTENSIONS OFF
)

target_include_directories(${CONVERTER_TGT_NAME} PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}/../engine
)

target_link_libraries(${CONVERTER_TGT_NAME}
	SDL2::SDL2
	ZLIB::ZLIB
	yaml-cpp::yaml-cpp
)
