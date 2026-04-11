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

# --- CombatEntity stat tests ---
set(TEST_COMBAT_ENTITY_TGT test-combat-entity)

add_executable(${TEST_COMBAT_ENTITY_TGT}
	test_combat_entity.cpp
)

set_target_properties(${TEST_COMBAT_ENTITY_TGT} PROPERTIES
	CXX_STANDARD 17
	CXX_STANDARD_REQUIRED ON
	CXX_EXTENSIONS OFF
)

target_include_directories(${TEST_COMBAT_ENTITY_TGT} PRIVATE
	${CMAKE_SOURCE_DIR}/src/engine
	${CMAKE_SOURCE_DIR}/src/game
)

target_link_libraries(${TEST_COMBAT_ENTITY_TGT} drpg-engine-lib)

add_test(NAME combat-entity COMMAND ${TEST_COMBAT_ENTITY_TGT})

# --- Player stats tests (links against engine library) ---
set(TEST_PLAYER_STATS_TGT test-player-stats)

add_executable(${TEST_PLAYER_STATS_TGT}
	test_player_stats.cpp
)

set_target_properties(${TEST_PLAYER_STATS_TGT} PROPERTIES
	CXX_STANDARD 17
	CXX_STANDARD_REQUIRED ON
	CXX_EXTENSIONS OFF
)

target_include_directories(${TEST_PLAYER_STATS_TGT} PRIVATE
	${CMAKE_SOURCE_DIR}/src/engine
	${CMAKE_SOURCE_DIR}/src/game
)

target_link_libraries(${TEST_PLAYER_STATS_TGT} drpg-engine-lib)

add_test(NAME player-stats COMMAND ${TEST_PLAYER_STATS_TGT})
