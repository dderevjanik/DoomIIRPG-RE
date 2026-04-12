#pragma once
class Text;

// -------------------
// Localization Class
// -------------------

class Applet;

class Localization
{
private:

public:
	Applet* app;  // Set in startup(), replaces CAppContainer::getInstance()->app

	static constexpr int MAXBUFFERS = 7;
	static constexpr int MAXTEXT = 15;
	static constexpr int MAX_STRING_ARGS = 50;
	static constexpr uint8_t C_LINE = '\x80';
	static constexpr uint8_t C_ELLIPSES = '\x85';
	static constexpr uint8_t C_RED_ING = 0xBC;
	static constexpr uint8_t C_BLUE_ING = 0xBD;
	static constexpr uint8_t C_GREEN_ING = 0xBE;
	static constexpr uint8_t C_CHECK = '\x87';
	static constexpr uint8_t C_MINIDASH = '\x88';
	static constexpr uint8_t C_CURSOR2 = '\x84';
	static constexpr uint8_t C_CURSOR = '\x8a';
	static constexpr uint8_t C_POINTER = '\x90';
	static constexpr uint8_t C_GREYLINE = '\x89';
	static constexpr uint8_t C_HEART = '\x8d';
	static constexpr uint8_t C_SHIELD = '\x8b';
	static constexpr uint8_t HYPHEN = '-';
	static constexpr uint8_t NEWLINE = '|';
	static constexpr uint8_t HARD_SPACE = 0xA0;

	Text* scratchBuffers[Localization::MAXBUFFERS];
	int bufferFlags;
	Text* dynamicArgs;
	int16_t argIndex[Localization::MAX_STRING_ARGS];
	int numTextArgs;
	bool selectLanguage;
	int defaultLanguage;
	int textSizes[Localization::MAXTEXT];
	int textCount[Localization::MAXTEXT];
	char** text;
	uint16_t** textMap;
	void* yamlData; // Legacy: single YAML::Node* for old strings.yaml format
	void* groupYamlData[Localization::MAXTEXT]; // Per-group DataNode* for split files

	// Constructor
	Localization();
	// Destructor
	~Localization();

	bool startup();
	bool loadFromYAML(const char* path);
	void loadGroupFromYAML(int language, int group);
	static bool isSpace(char c);
	static bool isDigit(char c);
	static char toLower(char c);
	static char toUpper(char c);
	Text* getSmallBuffer();
	Text* getFatalErrorBuffer();
	Text* getLargeBuffer();
	void freeAllBuffers();
	void unloadText(int index);
	void setLanguage(int language);
	void loadText(int index);
	void resetTextArgs();
	void addTextArg(char c);
	void addTextArg(int i);
	void addTextArg(Text* text, int i, int i2);
	void addTextArg(Text* text);
	void addTextIDArg(int16_t i);
	void addTextArg(int16_t i, int16_t i2);
	static constexpr int STRINGID(int16_t i, int16_t i2) {
		return i << 10 | i2;
	};
	void composeText(int i, Text* text);
	void composeText(int16_t n, int16_t n2, Text* text);
	void composeTextField(int i, Text* text);
	bool isEmptyString(int16_t i, int16_t i2);
	bool isEmptyString(int i);
	void getCharIndices(char c, int* i, int* i2);
};

// -----------
// Text Class
// -----------

class Text
{
private:

public:
	char* chars;
	int _length;
	int stringWidth;

	// Constructor
	Text(int countChars);
	// Destructor
	~Text();

	bool startup();
	int length();
	void setLength(int i);
	Text* deleteAt(int i, int i2);
	char charAt(int i);
	void setCharAt(char c, int i);
	Text* append(char c);
	Text* append(uint8_t c);
	Text* append(const char* c);
	Text* append(int i);
	Text* append(Text* t);
	Text* append(Text* t, int i);
	Text* append(Text* t, int i, int i2);
	Text* insert(char c, int i);
	Text* insert(uint8_t c, int i);
	Text* insert(int i, int i2);
	Text* insert(char* c, int i);
	Text* insert(char* c, int i, int i2, int i3);
	int findFirstOf(char c);
	int findFirstOf(char c, int i);
	int findLastOf(char c);
	int findLastOf(char c, int n);
	int findAnyFirstOf(char* c, int i);
	void substring(Text* t, int i);
	void substring(Text* t, int i, int i2);
	void dehyphenate();
	void dehyphenate(int i, int i2);
	void trim();
	void trim(bool b, bool b2);
	int wrapText(int i);
	int wrapText(int i, char c);
	int wrapText(int i, int i2, char c);
	int wrapText(int i, int i2, int i3, char c);
	int insertLineBreak(int i, int i2, char c);
	int getStringWidth();
	int getStringWidth(bool b);
	int getStringWidth(int i, int i2, bool b);
	int getNumLines();
	bool compareTo(Text* t);
	bool compareTo(char* str);
	void toLower();
	void toUpper();
	void dispose();
};
