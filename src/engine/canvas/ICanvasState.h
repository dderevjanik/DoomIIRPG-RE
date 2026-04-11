#ifndef __ICANVASSTATE_H__
#define __ICANVASSTATE_H__

class Canvas;
class Graphics;

// Abstract interface for Canvas game states.
// Each state handles its own enter/exit, update, render, and input logic.
class ICanvasState {
public:
	virtual ~ICanvasState() = default;

	// Called when transitioning INTO this state (after old state's onExit)
	virtual void onEnter(Canvas* canvas) = 0;

	// Called when transitioning OUT of this state (before new state's onEnter)
	virtual void onExit(Canvas* canvas) = 0;

	// Per-frame update logic (called from Canvas::run)
	virtual void update(Canvas* canvas) = 0;

	// Render logic (called from Canvas::backPaint)
	virtual void render(Canvas* canvas, Graphics* graphics) = 0;

	// Input handling (called from Canvas::handleEvent)
	// Returns true if the event was consumed
	virtual bool handleInput(Canvas* canvas, int key, int action) = 0;
};

#endif
