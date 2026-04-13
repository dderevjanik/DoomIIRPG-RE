#pragma once
class Image;
class Graphics;

class Applet;

class ComicBook
{
private:

public:
	Applet* app = nullptr;  // Set lazily, replaces CAppContainer::getInstance()->app
	int field_0x0 = 0;
	int field_0x4 = 0;
	int curX = 0;
	int curY = 0;
	bool field_0x10 = false;
	bool isLoaded = false;
	uint8_t field_0x12 = 0;
	uint8_t field_0x13 = 0;
	int comicBookIndex = 0;
	int iPhoneComicIndex = 0;
	int field_0x1c = 0;
	Image* imgComicBook[17] = {};
	Image* imgiPhoneComicBook[39] = {};
	int iPhonePage = 0;
	int cur_iPhonePage = 0;
	int page = 0;
	int curPage = 0;
	bool field_0x110 = false;
	uint8_t field_0x111 = 0;
	uint8_t field_0x112 = 0;
	uint8_t field_0x113 = 0;
	int endPoint = 0;
	bool field_0x118 = false;
	uint8_t field_0x119 = 0;
	uint8_t field_0x11a = 0;
	uint8_t field_0x11b = 0;
	int field_0x11c = 0;
	int begPoint = 0;
	int midPoint = 0;
	float accelerationX = 0.0f;
	float accelerationY = 0.0f;
	float accelerationZ = 0.0f;
	bool is_iPhoneComic = false;
	bool field_0x135 = false;
	uint8_t field_0x136 = 0;
	uint8_t field_0x137 = 0;
	int field_0x138 = 0;
	bool field_0x13c = false;
	uint8_t field_0x13d = 0;
	uint8_t field_0x13e = 0;
	uint8_t field_0x13f = 0;
	int field_0x140 = 0;
	float field_0x144 = 0.0f;
	bool drawExitButton = false;
	uint8_t field_0x149 = 0;
	uint8_t field_0x14a = 0;
	uint8_t field_0x14b = 0;
	int field_0x14c = 0;
	int exitBtnRect[4] = {};
	bool exitBtnHighlighted = false;
	uint8_t field_0x161 = 0;
	uint8_t field_0x162 = 0;
	uint8_t field_0x163 = 0;
	int field_0x164 = 0;
	int field_0x168 = 0;
	int field_0x16c = 0;
	int field_0x170 = 0;
	bool field_0x174 = false;
	uint8_t field_0x175 = 0;
	uint8_t field_0x176 = 0;
	uint8_t field_0x177 = 0;

	// Constructor
	ComicBook();
	// Destructor
	~ComicBook();

	void Draw(Graphics* graphics);
	void DrawLoading(Graphics* graphics);
	void loadImage(int index, bool vComic);
	void CheckImageExistence(Image* image);
	void DrawImage(Image* image, int a3, int a4, char a5, float alpha, char a7);

	void UpdateMovement();
	void UpdateTransition();

	void Touch(int x, int y, bool b);
	bool ButtonTouch(int x, int y);
	void TouchMove(int x, int y);
	void DeleteImages();
	void DrawExitButton(Graphics* graphics);
	void handleComicBookEvents(int key, int keyAction);
};
