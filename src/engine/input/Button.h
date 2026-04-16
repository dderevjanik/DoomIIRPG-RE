#pragma once
#include <span>
class Image;
class Graphics;

// ------------------------
// GuiRect Class
// ------------------------

class GuiRect
{
public:
	int x;
	int y;
	int w;
	int h;
	void Set(int x, int y, int w, int h);
	bool ContainsPoint(int x, int y);
};

// ---------------
// fmButton Class
// ---------------

class Applet;

class fmButton
{
private:

public:
	Applet* app = nullptr;  // Set lazily, replaces CAppContainer::getInstance()->app
	fmButton* next = nullptr;
	int buttonID = 0;
	int selectedIndex = 0;
	bool drawButton = false;
	bool highlighted = false;
	int normalRenderMode = 0;
	int highlightRenderMode = 0;
	bool drawTouchArea = false;
	Image* imgNormal = nullptr;
	Image* imgHighlight = nullptr;
	Image** ptrNormalImages = nullptr;
	Image** ptrHighlightImages = nullptr;
	int normalIndex = 0;
	int highlightIndex = 0;
	int centerX = 0;
	int centerY = 0;
	int highlightCenterX = 0;
	int highlightCenterY = 0;
	float normalRed = 0.0f;
	float normalGreen = 0.0f;
	float normalBlue = 0.0f;
	float normalAlpha = 0.0f;
	float highlightRed = 0.0f;
	float highlightGreen = 0.0f;
	float highlightBlue = 0.0f;
	float highlightAlpha = 0.0f;
	short soundResID = 0;
	GuiRect touchArea = {};
	GuiRect touchAreaDrawing = {}; // Port: New

	// Constructor
	fmButton(int buttonID, int x, int y, int w, int h, int soundResID);
	// Destructor
	~fmButton();

	void SetImage(Image* image, bool center);
	void SetImage(Image** ptrImages, int imgIndex, bool center);
	void SetHighlightImage(Image* imgHighlight, bool center);
	void SetHighlightImage(Image** ptrImgsHighlight, int imgHighlightIndex, bool center);
	void SetGraphic(int index);
	void SetTouchArea(int x, int y, int w, int h);
	void SetTouchArea(int x, int y, int w, int h, bool drawing); // Port: new
	void SetHighlighted(bool highlighted);
	void Render(Graphics* graphics);
};

// ------------------------
// ButtonDef — data-driven button definition
// ------------------------

struct ButtonDef {
	int id;
	int x, y, w, h;
	int soundResID;
};

// ------------------------
// fmButtonContainer Class
// ------------------------

class fmButtonContainer
{
private:

public:
	fmButton* next = nullptr;
	fmButton* prev = nullptr;

	// Constructor
	fmButtonContainer();
	// Destructor
	~fmButtonContainer();

	void AddButton(fmButton* button);
	fmButton* GetButton(int buttonID);
	fmButton* GetTouchedButton(int x, int y);
	int GetTouchedButtonID(int x, int y);
	int GetHighlightedButtonID();
	void HighlightButton(int x, int y, bool highlighted);
	void SetGraphic(int index);
	void FlipButtons();
	void Render(Graphics* graphics);
};

// Create buttons from a ButtonDef array and add them to a container
void createButtons(fmButtonContainer* container, std::span<const ButtonDef> defs);

// ---------------------
// fmScrollButton Class
// ---------------------

class fmScrollButton
{
private:

public:
	uint8_t enabled;
	Image* imgBar;
	Image* imgBarTop;
	Image* imgBarMiddle;
	Image* imgBarBottom;
	uint8_t barTouched;
	bool isVertical;
	GuiRect barRect;
	GuiRect boxRect;
	uint8_t contentDragging;
	int visibleSize; // guessed
	int contentSize; // guessed
	int scrollOffset; // guessed
	int thumbPosition; // guessed
	int thumbSize; // guessed
	float scrollRatio; // guessed
	int thumbDragOffset; // guessed
	int touchStartPos; // guessed
	int touchStartScrollOffset; // guessed
	short soundResID;

	// Constructor
	fmScrollButton(int x, int y, int w, int h, bool b, int soundResID);
	// Destructor
	~fmScrollButton();

	//bool startup();
	void SetScrollBarImages(Image* imgBar, Image* imgBarTop, Image* imgBarMiddle, Image* imgBarBottom);
	void SetScrollBox(int x, int y, int w, int h, int totalContentSize);
	void SetScrollBox(int x, int y, int w, int h, int totalContentSize, int thumbSizeOverride);
	void SetContentTouchOffset(int x, int y);
	void UpdateContent(int x, int y);
	void SetTouchOffset(int x, int y);
	void Update(int x, int y);
	void Render(Graphics* graphics);
};

// ------------------
// fmSwipeArea Class
// ------------------

class fmSwipeArea
{
private:

public:
	//typedef int SwipeDir;
	enum SwipeDir {Null = -1, Left, Right, Down, Up};

	bool enable = false;
	bool touched = false;
	bool drawTouchArea = false;
	GuiRect rect = {};
	int begX = 0;
	int begY = 0;
	int curX = 0;
	int curY = 0;
	int unused_0x22 = 0; // never used
	int endX = 0;
	int endY = 0;

	// Constructor
	fmSwipeArea(int x, int y, int w, int h, int endX, int endY);
	// Destructor
	~fmSwipeArea();

	int UpdateSwipe(int x, int y, SwipeDir* swDir);
	void Render(Graphics* graphics);
};
