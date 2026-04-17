#pragma once
#include <cstdint>

class Applet;
class Canvas;
class EntityDef;
class Graphics;
class ScriptThread;
class Text;
class fmButtonContainer;

// Owns dialog state: paginated dialog rendering, help message queue, and
// the lifecycle hooks (startDialog / closeDialog) driven from scripts, states,
// the player, and menu input.
//
// Canvas retains cross-cutting state: dialogBuffer (shared large text buffer),
// dialogRect, dialogStyleDefs, layout fields (SCR_CX, screenRect, hudRect),
// state dispatch (setState / state / oldState / combatDone), drawScrollBar,
// and clearLeftSoftKey. DialogManager reaches back via a Canvas* argument.
class DialogManager
{
public:
	Applet* app = nullptr;

	// Paginated dialog body
	short dialogIndexes[1024] = {};
	int numDialogLines = 0;
	int currentDialogLine = 0;
	int dialogViewLines = 0;
	int dialogLineStartTime = 0;
	int dialogStartTime = 0;
	int dialogTypeLineIdx = 0;
	int dialogStyle = 0;
	int dialogType = 0;
	int dialogFlags = 0;
	bool dialogResumeScriptAfterClosed = false;
	bool dialogResumeMenu = false;
	bool dialogClosing = false;
	ScriptThread* dialogThread = nullptr;
	EntityDef* dialogItem = nullptr;

	// Loot-style dialog decoration
	bool showingLoot = false;
	int specialLootIcon = 0;

	// Help-message queue (deferred dialog show)
	int helpMessageTypes[16] = {};
	int helpMessageInts[16] = {};
	void* helpMessageObjs[16] = {};
	char helpMessageThreads[16] = {};
	int numHelpMessages = 0;

	// Touch buttons shown during dialog (owned by DialogManager; constructed
	// by Canvas::startup alongside other button containers).
	fmButtonContainer* m_dialogButtons = nullptr;

	explicit DialogManager(Applet* app) : app(app) {}

	// Paginated dialog rendering (replaces Canvas::dialogState).
	void render(Canvas* canvas, Graphics* graphics);

	// Lifecycle.
	void close(Canvas* canvas, bool skipDialog);
	void prepare(Canvas* canvas, Text* text, int dialogStyle, int dialogFlags);

	// startDialog overloads mirror the original Canvas API.
	void startDialog(Canvas* canvas, ScriptThread* scriptThread, short n, int n2, int n3);
	void startDialog(Canvas* canvas, ScriptThread* scriptThread, short n, short n2, int n3, int n4, bool b);
	void startDialog(Canvas* canvas, ScriptThread* scriptThread, Text* text, int n, int n2);
	void startDialog(Canvas* canvas, ScriptThread* dialogThread, Text* text, int n, int n2, bool dialogResumeScriptAfterClosed);

	// Help message queue — caller enqueues, dequeue runs when state allows.
	void dequeueHelpDialog(Canvas* canvas);
	void dequeueHelpDialog(Canvas* canvas, bool b);
	void enqueueHelpDialog(Canvas* canvas, short n);
	bool enqueueHelpDialog(Canvas* canvas, short n, short n2, uint8_t b);
	bool enqueueHelpDialog(Canvas* canvas, Text* text);
	bool enqueueHelpDialog(Canvas* canvas, Text* text, int n);
	void enqueueHelpDialog(Canvas* canvas, EntityDef* entityDef);
};
