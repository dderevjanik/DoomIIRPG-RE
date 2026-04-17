#pragma once

class Applet;
class Canvas;
class Graphics;
class Image;
class fmButtonContainer;

// Owns story/intro/credits/epilogue rendering state. Covers:
// - paginated story pages (intro, pre-level exposition)
// - vertically-scrolling credits and epilogue text
// - the animated intro movie (prolog.bmp + scrolling text)
//
// Canvas retains the cross-cutting state: dialogBuffer (shared text buffer),
// stateVars, screenRect/displayRect/cinRect, fade/* fields. This class reaches
// back via the Canvas* argument passed to each method.
class StoryRenderer
{
public:
	Applet* app = nullptr;

	// Paginated story pages (intro / character-selection intro)
	int storyX = 0;
	int storyY = 0;
	int storyPage = 0;
	int storyTotalPages = 0;
	int storyIndexes[4] = {};

	// Vertically-scrolling credits and epilogue
	int scrollingTextStart = 0;
	int scrollingTextEnd = 0;
	int scrollingTextMSLine = 0;
	int scrollingTextLines = 0;
	int scrollingTextSpacing = 0;
	bool scrollingTextDone = false;
	int scrollingTextFontHeight = 0;
	int scrollingTextSpacingHeight = 0;

	// Shared intro prolog art (used by playIntroMovie and epilogue).
	Image* imgProlog = nullptr;

	// Touch buttons shown during the story pager (back / next / skip).
	// Created by Canvas::startup so layout is consistent with other button
	// containers; owned by this class so its lifetime matches the state.
	fmButtonContainer* m_storyButtons = nullptr;

	explicit StoryRenderer(Applet* app) : app(app) {}

	// Prologue/epilogue loading and disposal.
	void loadPrologueText(Canvas* canvas);
	void loadEpilogueText(Canvas* canvas);
	void disposeIntro(Canvas* canvas);
	void disposeEpilogue(Canvas* canvas);

	// Scrolling text (credits, epilogue, intro movie text).
	void initScrollingText(Canvas* canvas, short i, short i2, bool dehyphenate, int spacingHeight, int numLines, int textMSLine);
	void drawCredits(Canvas* canvas, Graphics* graphics);
	void drawScrollingText(Canvas* canvas, Graphics* graphics);

	// Paginated story UI.
	void changeStoryPage(Canvas* canvas, int delta);
	void drawStory(Canvas* canvas, Graphics* graphics);
	void handleStoryInput(Canvas* canvas, int key, int action);

	// Intro movie.
	void playIntroMovie(Canvas* canvas, Graphics* graphics);
	void exitIntroMovie(Canvas* canvas, bool skipExit);
};
