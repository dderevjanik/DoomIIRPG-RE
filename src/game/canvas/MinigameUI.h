#pragma once

class Applet;
class Canvas;
class Graphics;
class Image;
class ScriptThread;

// Shared minigame UI state + helpers used by HackingGame, SentryBotGame,
// VendingMachine and the armor-repair flow. Owns the scroll/help screen
// state and the in-flight armor-repair script thread.
class MinigameUI
{
public:
	Applet* app = nullptr;

	int miniGameHelpScrollPosition = 0;
	int helpTextNumberOfLines = 0;
	ScriptThread* armorRepairThread = nullptr;
	bool repairingArmor = false;

	explicit MinigameUI(Applet* app) : app(app) {}

	// Called once during startup to load textures for every minigame +
	// help screen. Assets are written to each minigame's Image fields.
	void loadImages();

	// HUD overlay shown during the target-practice minigame.
	void drawTargetPracticeScore(Graphics* graphics);

	// Help-screen scroll state.
	void initHelpScreen();
	void drawHelpScreen(Canvas* canvas, Graphics* graphics, int titleStr, int bodyStr, Image* image);
	void drawHelpText(Canvas* canvas, Graphics* graphics, int titleStr, int bodyStr);
	void handleHelpScreenScroll(int delta);

	// Armor-repair minigame lifecycle.
	bool startArmorRepair(Canvas* canvas, ScriptThread* thread);
	void endArmorRepair(Canvas* canvas);

	// Called when a minigame finishes to apply stat bonuses etc.
	void evaluateResults(int result);
};
