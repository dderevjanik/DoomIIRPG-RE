#ifndef __SAVINGSTATE_H__
#define __SAVINGSTATE_H__

#include "ICanvasState.h"

class SavingState : public ICanvasState {
public:
	void onEnter(Canvas* canvas) override;
	void onExit(Canvas* canvas) override;
	void update(Canvas* canvas) override;
	void render(Canvas* canvas, Graphics* graphics) override;
	bool handleInput(Canvas* canvas, int key, int action) override;
};

#endif
