#pragma once
#include <utility>
#include <string>
//------------------------------------------------------------------------------------------------------------------------------------------
// Enum representing an input source on a non-generic game controller recognized by SDL (axis or button)
//------------------------------------------------------------------------------------------------------------------------------------------
enum class GamepadInput : uint8_t {
	BTN_A,
	BTN_B,
	BTN_X,
	BTN_Y,
	BTN_BACK,
	BTN_GUIDE,
	BTN_START,
	BTN_LEFT_STICK,
	BTN_RIGHT_STICK,
	BTN_LEFT_SHOULDER,
	BTN_RIGHT_SHOULDER,
	BTN_DPAD_UP,
	BTN_DPAD_DOWN,
	BTN_DPAD_LEFT,
	BTN_DPAD_RIGHT,
	AXIS_LEFT_X,
	AXIS_LEFT_Y,
	AXIS_RIGHT_X,
	AXIS_RIGHT_Y,
	AXIS_TRIG_LEFT,
	AXIS_TRIG_RIGHT,
	BTN_LAXIS_UP,		// L-Axis Up
	BTN_LAXIS_DOWN,		// L-Axis Down
	BTN_LAXIS_LEFT,		// L-Axis Left
	BTN_LAXIS_RIGHT,	// L-Axis Right
	BTN_RAXIS_UP,		// R-Axis Up
	BTN_RAXIS_DOWN,		// R-Axis Down
	BTN_RAXIS_LEFT,		// R-Axis Left
	BTN_RAXIS_RIGHT,	// R-Axis Right
	// N.B: must keep last for 'NUM_GAMEPAD_INPUTS' constant!
	INVALID
};

static constexpr uint8_t NUM_GAMEPAD_INPUTS = std::to_underlying(GamepadInput::INVALID);

// Direction for a joystick hat (d-pad)
enum class JoyHatDir : uint8_t {
	Up = 0,
	Down = 1,
	Left = 2,
	Right = 3
};

// Holds a joystick hat (d-pad) direction and hat number
union JoyHat {
	struct {
		JoyHatDir   hatDir;
		uint8_t     hatNum;
	} fields;

	uint16_t bits;

	inline JoyHat() noexcept : bits() {}
	inline JoyHat(const uint16_t bits) noexcept : bits(bits) {}

	inline JoyHat(const JoyHatDir dir, const uint8_t hatNum) noexcept : bits() {
		fields.hatDir = dir;
		fields.hatNum = hatNum;
	}

	inline operator uint16_t() const noexcept { return bits; }

	inline bool operator == (const JoyHat& other) const noexcept { return (bits == other.bits); }
	inline bool operator != (const JoyHat& other) const noexcept { return (bits != other.bits); }
};

//static_assert(sizeof(JoyHat) == 2);

// Holds the current state of a generic joystick axis
struct JoystickAxis {
	uint32_t    axis;
	float       value;
};
//---------------

extern char buttonNames[][NUM_GAMEPAD_INPUTS];

static constexpr int KEYBINDS_MAX = 10;
static constexpr int IS_MOUSE_BUTTON = 0x100000;
static constexpr int IS_CONTROLLER_BUTTON = 0x200000;
struct keyMapping_t {
	int avk_action;
	int keyBinds[KEYBINDS_MAX];
};

static constexpr int KEY_MAPPIN_MAX = 16;
extern keyMapping_t keyMapping[KEY_MAPPIN_MAX];
extern keyMapping_t keyMappingTemp[KEY_MAPPIN_MAX];
extern keyMapping_t keyMappingDefault[KEY_MAPPIN_MAX];

extern int      gDeadZone;
extern int		gVibrationIntensity;
extern float    gBegMouseX;
extern float    gBegMouseY;
extern float    gCurMouseX;
extern float    gCurMouseY;

// Game action / input-event enum. Originally derived from BREW Application Virtual
// Keys (AVK_*) but in this engine the values are used as game actions (e.g. AVK_FIRE
// via AVK_SELECT, menu-open via AVK_MENUOPEN) and as the index range for digit entry
// (AVK_0 + n with n in 0..9). The high bits (>= AVK_MENU_UP = 0x40) are bitmask
// flags OR'd onto the low value to disambiguate menu vs gameplay events.
enum _AVKType {
	AVK_UNDEFINED = -1,

	AVK_0,		// digit-entry base; AVK_0..AVK_0+9 are the digit slots
	AVK_1,		// reserved (slot for digit 1)
	AVK_2,		// reserved
	AVK_3,		// reserved
	AVK_4,		// reserved
	AVK_5,		// reserved
	AVK_6,		// reserved
	AVK_7,		// reserved
	AVK_8,		// reserved
	AVK_9,		// reserved
	AVK_STAR,
	AVK_POUND,

	AVK_POWER,	// reserved (slot retained to keep AVK_SELECT == 13 stable)
	AVK_SELECT,	// confirm / KEY_OK

	AVK_UP,
	AVK_DOWN,
	AVK_LEFT,
	AVK_RIGHT,

	AVK_CLR,	// back / clear

	AVK_SOFT1,	// left soft key
	AVK_SOFT2,	// right soft key

	AVK_UNK = 26,	// touch / unknown event suppressor (see Canvas::handleEvent)

	// New Types Only on port
	AVK_MENUOPEN = 30,
	AVK_AUTOMAP,
	AVK_MOVELEFT,
	AVK_MOVERIGHT,
	AVK_PREVWEAPON,
	AVK_NEXTWEAPON,
	AVK_PASSTURN,
	AVK_BOTDISCARD,
	

	// New Flags Menu Only on port
	AVK_MENU_UP = 0x40,
	AVK_MENU_DOWN = 0x80,
	AVK_MENU_PAGE_UP = 0x100,
	AVK_MENU_PAGE_DOWN = 0x200,
	AVK_MENU_SELECT = 0x400,
	AVK_MENU_OPEN = 0x800,
	AVK_MENU_NUMBER = 0x2000,
	AVK_ITEMS_INFO = 0x4000,
	AVK_DRINKS = 0x8000,
	AVK_PDA = 0x10000
};

extern void controllerVibrate(int duration_ms) noexcept;

// Convert between input bind codes and human-readable names ("W", "pad_a", "mouse_left", "none").
// Used by both the save-game settings round-trip and the controls.yaml defaults loader.
const char* inputCodeToName(int code);
int inputCodeFromName(const std::string& name);

// Action slot names matching keyMappingDefault[KEY_MAPPIN_MAX] order.
// Used as keys in controls.yaml (e.g. "move_forward", "menu") and in the
// settings file. Length = KEY_MAPPIN_MAX.
extern const char* const keyBindingNames[];

class Applet;
class SDLGL;

class Input
{
private:

public:

	Applet* app; // Set in init(), replaces CAppContainer::getInstance()->app
	bool headless = false;
	SDLGL* sdlGL = nullptr;

	// Constructor
	Input();
	// Destructor
	~Input();

	void init();
	void unBind(int* keyBinds, int keycode);
	void setBind(int* keyBinds, int keycode);
	void setInputBind(int scancode);
	void handleEvents() noexcept;
	void consumeEvents() noexcept;
};
