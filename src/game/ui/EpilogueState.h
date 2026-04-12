#ifndef __EPILOGUESTATE_H__
#define __EPILOGUESTATE_H__

#include "ICanvasState.h"

class EpilogueState : public ICanvasState {
public:
	void onEnter(Canvas* canvas) override;
	void onExit(Canvas* canvas) override;
	void update(Canvas* canvas) override;
	void render(Canvas* canvas, Graphics* graphics) override;
	bool handleInput(Canvas* canvas, int key, int action) override;
};

#endif
