#pragma once
class ScriptThread;
class OutputStream;
class InputStream;

class Applet;

class LerpSprite
{
private:

public:
    Applet* app;  // Set lazily, replaces CAppContainer::getInstance()->app
    int touchMe;
    ScriptThread* thread;
    int travelTime;
    int startTime;
    int hSprite;
    int srcX;
    int srcY;
    int srcZ;
    int dstX;
    int dstY;
    int dstZ;
    int height;
    int dist;
    int srcScale;
    int dstScale;
    int flags;

	// Constructor
	LerpSprite();
	// Destructor
	~LerpSprite();

    void saveState(OutputStream* OS);
    void calcDist();
    void loadState(InputStream* IS);
};
