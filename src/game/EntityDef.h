#ifndef __ENTITYDEF_H__
#define __ENTITYDEF_H__

class EntityDef;

// -----------------------
// EntityDefManager Class
// -----------------------

class EntityDefManager {
  private:
	EntityDef* list;
	int numDefs;

	bool loadFromBinary();
	bool loadFromINI(const char* path);

  public:
	// Constructor
	EntityDefManager();
	// Destructor
	~EntityDefManager();

	bool startup();
	void exportToINI(const char* path);
	EntityDef* find(int eType, int eSubType);
	EntityDef* find(int eType, int eSubType, int parm);
	EntityDef* lookup(int tileIndex);
};

// ----------------
// EntityDef Class
// ----------------

class EntityDef {
  private:
  public:
	// Render flag constants (bitmask)
	static constexpr uint32_t RFLAG_NONE = 0;
	static constexpr uint32_t RFLAG_FLOATER = 0x1;      // Floats (no legs), uses floating anim path
	static constexpr uint32_t RFLAG_GUN_FLARE = 0x2;    // Shows gun flash on attack
	static constexpr uint32_t RFLAG_SPECIAL_BOSS = 0x4; // Uses special boss rendering path

	int16_t tileIndex;
	int16_t name;
	int16_t longName;
	int16_t description;
	uint8_t eType;
	uint8_t eSubType;
	uint8_t parm;
	uint8_t touchMe;
	uint32_t renderFlags; // Bitmask of RFLAG_* constants

	// Constructor
	EntityDef();

	// Render flag helpers
	bool hasRenderFlag(uint32_t flag) const { return (renderFlags & flag) != 0; }
};

#endif
