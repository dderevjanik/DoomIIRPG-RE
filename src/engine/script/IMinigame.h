#ifndef __IMINIGAME_H__
#define __IMINIGAME_H__

class Graphics;

// Abstract interface for minigames (hacking, sentrybot, vending, etc.)
// Each minigame handles its own rendering, input, and touch events.
class IMinigame {
public:
	virtual ~IMinigame() = default;

	// Render the minigame (called each frame when ST_MINI_GAME is active)
	virtual void updateGame(Graphics* graphics) = 0;

	// Keyboard/controller input
	virtual void handleInput(int action) = 0;

	// Touch events
	virtual void touchStart(int x, int y) = 0;
	virtual void touchMove(int x, int y) = 0;
	virtual void touchEnd(int x, int y) = 0;

	// Play standalone from main menu (--minigame flag)
	virtual void playFromMainMenu() = 0;

	// Force a win (for skipMinigames cheat)
	virtual void forceWin() = 0;

	// End the game: 0=loss, 1=win, 2=quit
	virtual void endGame(int n) = 0;
};

#endif
