#pragma once
class Image;
class Graphics;

class Applet;

class ComicBook
{
private:

public:
	Applet* app = nullptr;  // Set lazily, replaces CAppContainer::getInstance()->app
	int touchStartX = 0;
	int touchStartY = 0;
	int curX = 0;
	int curY = 0;
	bool isTouching = false;
	bool isLoaded = false;
	uint8_t pad_0x12 = 0;
	uint8_t pad_0x13 = 0;
	int comicBookIndex = 0;
	int iPhoneComicIndex = 0;
	int loadingCounter = 0; // guessed — incremented in DrawLoading, never read
	Image* imgComicBook[17] = {};
	Image* imgiPhoneComicBook[39] = {};
	int iPhonePage = 0;
	int cur_iPhonePage = 0;
	int page = 0;
	int curPage = 0;
	bool isTransitioning = false;
	uint8_t pad_0x111 = 0;
	uint8_t pad_0x112 = 0;
	uint8_t pad_0x113 = 0;
	int endPoint = 0;
	bool isSnappingBack = false;
	uint8_t pad_0x119 = 0;
	uint8_t pad_0x11a = 0;
	uint8_t pad_0x11b = 0;
	int unused_0x11c = 0; // set in constructor but never read
	int begPoint = 0;
	int midPoint = 0;
	float accelerationX = 0.0f;
	float accelerationY = 0.0f;
	float accelerationZ = 0.0f;
	bool is_iPhoneComic = false;
	bool isFadingOut = false; // guessed
	uint8_t pad_0x136 = 0;
	uint8_t pad_0x137 = 0;
	int fadeCounter = 0; // guessed
	bool isOrientationSwitch = false; // guessed
	uint8_t pad_0x13d = 0;
	uint8_t pad_0x13e = 0;
	uint8_t pad_0x13f = 0;
	int transitionCounter = 0; // guessed
	float transitionOffset = 0.0f; // guessed
	bool drawExitButton = false;
	uint8_t pad_0x149 = 0;
	uint8_t pad_0x14a = 0;
	uint8_t pad_0x14b = 0;
	int frameCounter = 0; // guessed
	int exitBtnRect[4] = {};
	bool exitBtnHighlighted = false;
	uint8_t pad_0x161 = 0;
	uint8_t pad_0x162 = 0;
	uint8_t pad_0x163 = 0;
	int unused_0x164 = 0; // set in constructor but never read
	int unused_0x168 = 0; // set in constructor but never read
	int unused_0x16c = 0; // set in constructor but never read
	int unused_0x170 = 0; // set in constructor but never read
	bool exitCancelled = false; // guessed — never set to true in current code
	uint8_t pad_0x175 = 0;
	uint8_t pad_0x176 = 0;
	uint8_t pad_0x177 = 0;

	// Constructor
	ComicBook();
	// Destructor
	~ComicBook();

	void Draw(Graphics* graphics);
	void DrawLoading(Graphics* graphics);
	void loadImage(int index, bool vComic);
	void CheckImageExistence(Image* image);
	void DrawImage(Image* image, int posX, int posY, char rotated, float alpha, char flipped);

	void UpdateMovement();
	void UpdateTransition();

	void Touch(int x, int y, bool b);
	bool ButtonTouch(int x, int y);
	void TouchMove(int x, int y);
	void DeleteImages();
	void DrawExitButton(Graphics* graphics);
	void handleComicBookEvents(int key, int keyAction);
};
