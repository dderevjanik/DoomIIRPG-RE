set(TEST_MAPDATA_TGT test-mapdata-roundtrip)

add_executable(${TEST_MAPDATA_TGT}
	test_mapdata_roundtrip.cpp
	${CMAKE_SOURCE_DIR}/src/converter/MapDataIO.cpp
	${CMAKE_SOURCE_DIR}/src/converter/BSPCompiler.cpp
)

set_target_properties(${TEST_MAPDATA_TGT} PROPERTIES
	CXX_STANDARD 23
	CXX_STANDARD_REQUIRED ON
	CXX_EXTENSIONS OFF
)

target_include_directories(${TEST_MAPDATA_TGT} PRIVATE
	${CMAKE_SOURCE_DIR}/src/converter
)

target_link_libraries(${TEST_MAPDATA_TGT} yaml-cpp)

add_test(NAME mapdata-roundtrip COMMAND ${TEST_MAPDATA_TGT})

# --- CombatEntity stat tests ---
set(TEST_COMBAT_ENTITY_TGT test-combat-entity)

add_executable(${TEST_COMBAT_ENTITY_TGT}
	test_combat_entity.cpp
)

set_target_properties(${TEST_COMBAT_ENTITY_TGT} PROPERTIES
	CXX_STANDARD 23
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
	CXX_STANDARD 23
	CXX_STANDARD_REQUIRED ON
	CXX_EXTENSIONS OFF
)

target_include_directories(${TEST_PLAYER_STATS_TGT} PRIVATE
	${CMAKE_SOURCE_DIR}/src/engine
	${CMAKE_SOURCE_DIR}/src/game
)

target_link_libraries(${TEST_PLAYER_STATS_TGT} drpg-engine-lib)

add_test(NAME player-stats COMMAND ${TEST_PLAYER_STATS_TGT})

# --- Game math tests ---
set(TEST_GAME_MATH_TGT test-game-math)
add_executable(${TEST_GAME_MATH_TGT} test_game_math.cpp)
set_target_properties(${TEST_GAME_MATH_TGT} PROPERTIES CXX_STANDARD 23 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
target_link_libraries(${TEST_GAME_MATH_TGT} drpg-engine-lib)
add_test(NAME game-math COMMAND ${TEST_GAME_MATH_TGT})

# --- Script VM stack tests ---
set(TEST_SCRIPT_STACK_TGT test-script-stack)
add_executable(${TEST_SCRIPT_STACK_TGT} test_script_stack.cpp)
set_target_properties(${TEST_SCRIPT_STACK_TGT} PROPERTIES CXX_STANDARD 23 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
target_link_libraries(${TEST_SCRIPT_STACK_TGT} drpg-engine-lib)
add_test(NAME script-stack COMMAND ${TEST_SCRIPT_STACK_TGT})

# --- Player inventory tests ---
set(TEST_PLAYER_INV_TGT test-player-inventory)
add_executable(${TEST_PLAYER_INV_TGT} test_player_inventory.cpp)
set_target_properties(${TEST_PLAYER_INV_TGT} PROPERTIES CXX_STANDARD 23 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
target_link_libraries(${TEST_PLAYER_INV_TGT} drpg-engine-lib)
add_test(NAME player-inventory COMMAND ${TEST_PLAYER_INV_TGT})

# --- Script VM opcode tests ---
set(TEST_SCRIPT_VM_TGT test-script-vm)
add_executable(${TEST_SCRIPT_VM_TGT} test_script_vm.cpp)
set_target_properties(${TEST_SCRIPT_VM_TGT} PROPERTIES CXX_STANDARD 23 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
target_link_libraries(${TEST_SCRIPT_VM_TGT} drpg-engine-lib)
add_test(NAME script-vm COMMAND ${TEST_SCRIPT_VM_TGT})
