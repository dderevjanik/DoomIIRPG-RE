#ifndef __LOADINGSTATE_H__
#define __LOADINGSTATE_H__

#include "ICanvasState.h"

class LoadingState : public ICanvasState {
public:
	void onEnter(Canvas* canvas) override;
	void onExit(Canvas* canvas) override;
	void update(Canvas* canvas) override;
	void render(Canvas* canvas, Graphics* graphics) override;
	bool handleInput(Canvas* canvas, int key, int action) override;
};

#endif
