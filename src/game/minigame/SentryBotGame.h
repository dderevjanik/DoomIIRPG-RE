#pragma once
class ScriptThread;
class Image;
class fmButtonContainer;
class Graphics;
class Applet;

#include "IMinigame.h"

class SentryBotGame : public IMinigame
{
private:
	static bool wasTouched;

public:
	Applet* app; // Set lazily, replaces CAppContainer::getInstance()->app
	int touchMe;
	ScriptThread* callingThread;
	Image* imgHelpScreenAssets;
	Image* imgGameAssets;
	Image* imgMatrixSkip_BG;
	Image* imgButton;
	Image* imgButton2;
	Image* imgButton3;
	uint32_t skipCode;
	int gameBoard[16];
	int solution[4];
	int usersGuess[4];
	int player_cursor;
	int answer_cursor;
	int bot_selection_cursor;
	int* stateVars;
	short botType;
	int timeSinceLastCursorMove;
	bool failedEarly;
	bool gamePlayedFromMainMenu;
	fmButtonContainer* m_sentryBotButtons;
	Image* imgSubmit;
	Image* imgUnk1;
	Image* imgDelete;
	bool touched;
	int currentPress_x;
	int currentPress_y;

	// Constructor
	SentryBotGame();
	// Destructor
	~SentryBotGame();

	void playFromMainMenu() override;
	void setupGlobalData();
	void initGame(ScriptThread* scriptThread, short botType);
	void handleInput(int action) override;
	void updateGame(Graphics* graphics) override;
	void drawFailureScreen(Graphics* graphics);
	void drawSuccessScreen(Graphics* graphics);
	void drawHelpScreen(Graphics* graphics);
	void drawGameScreen(Graphics* graphics);

	void drawPlayersGuess(int centerX, int topY, bool showCursor, Text* text, Graphics* graphics);
	void drawCursor(int x, int y, bool blinking, Graphics* graphics);
	bool playerHasWon();
	bool playerCouldStillWin();
	void forceWin() override;
	void awardSentryBot(int weaponIdx);
	void endGame(int result) override;
	void touchStart(int pressX, int pressY) override;
	void touchMove(int pressX, int pressY) override;
	void touchEnd(int pressX, int pressY) override;
	void handleTouchForHelpScreen(int pressX, int pressY);
	void handleTouchForGame(int pressX, int pressY);
};
