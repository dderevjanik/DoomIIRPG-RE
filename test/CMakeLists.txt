set(TEST_MAPDATA_TGT test-mapdata-roundtrip)

add_executable(${TEST_MAPDATA_TGT}
	test_mapdata_roundtrip.cpp
	${CMAKE_SOURCE_DIR}/src/editor/MapDataIO.cpp
	${CMAKE_SOURCE_DIR}/src/editor/BSPCompiler.cpp
)

set_target_properties(${TEST_MAPDATA_TGT} PROPERTIES
	CXX_STANDARD 17
	CXX_STANDARD_REQUIRED ON
	CXX_EXTENSIONS OFF
)

target_include_directories(${TEST_MAPDATA_TGT} PRIVATE
	${CMAKE_SOURCE_DIR}/src/editor
)

target_link_libraries(${TEST_MAPDATA_TGT} yaml-cpp)

add_test(NAME mapdata-roundtrip COMMAND ${TEST_MAPDATA_TGT})
