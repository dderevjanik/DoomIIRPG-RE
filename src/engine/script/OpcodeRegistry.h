#pragma once
#include <cstdint>
#include <cstring>

class ScriptThread;

// Handler function for a script opcode.
// Returns: 0 = continue execution (like break), 1 = done (return), 2 = paused/yielded
// The handler reads arguments from thread via getByteArg()/getShortArg()/etc.
// and manipulates state via push()/pop()/IP.
typedef int (*OpcodeHandler)(ScriptThread* thread);

// Registry for extension script opcodes (128-254).
// Base opcodes (0-97) remain in the ScriptThread switch.
// Opcode 255 is reserved for EV_END.
class OpcodeRegistry {
public:
	static constexpr int EXT_OPCODE_BASE = 128;
	static constexpr int EXT_OPCODE_MAX = 254;
	static constexpr int EXT_OPCODE_COUNT = EXT_OPCODE_MAX - EXT_OPCODE_BASE + 1; // 127 slots

	OpcodeRegistry() {
		std::memset(handlers, 0, sizeof(handlers));
		std::memset(names, 0, sizeof(names));
	}

	// Register a handler for an extension opcode (128-254).
	// Returns true on success, false if opcode out of range or already registered.
	bool registerOpcode(uint8_t opcode, OpcodeHandler handler, const char* name = nullptr) {
		if (opcode < EXT_OPCODE_BASE || opcode > EXT_OPCODE_MAX) return false;
		int idx = opcode - EXT_OPCODE_BASE;
		if (handlers[idx] != nullptr) return false; // Already registered
		handlers[idx] = handler;
		names[idx] = name;
		return true;
	}

	// Unregister an opcode handler.
	void unregisterOpcode(uint8_t opcode) {
		if (opcode < EXT_OPCODE_BASE || opcode > EXT_OPCODE_MAX) return;
		int idx = opcode - EXT_OPCODE_BASE;
		handlers[idx] = nullptr;
		names[idx] = nullptr;
	}

	// Look up handler for an opcode. Returns nullptr if not registered.
	OpcodeHandler getHandler(uint8_t opcode) const {
		if (opcode < EXT_OPCODE_BASE || opcode > EXT_OPCODE_MAX) return nullptr;
		return handlers[opcode - EXT_OPCODE_BASE];
	}

	// Get name for an opcode (for debug/error messages).
	const char* getName(uint8_t opcode) const {
		if (opcode < EXT_OPCODE_BASE || opcode > EXT_OPCODE_MAX) return nullptr;
		return names[opcode - EXT_OPCODE_BASE];
	}

	// Check if an opcode has a handler registered.
	bool hasHandler(uint8_t opcode) const {
		return getHandler(opcode) != nullptr;
	}

private:
	OpcodeHandler handlers[EXT_OPCODE_COUNT];
	const char* names[EXT_OPCODE_COUNT];
};
