#ifndef __LOOTINGSTATE_H__
#define __LOOTINGSTATE_H__

#include "ICanvasState.h"

class LootingState : public ICanvasState {
public:
	void onEnter(Canvas* canvas) override;
	void onExit(Canvas* canvas) override;
	void update(Canvas* canvas) override;
	void render(Canvas* canvas, Graphics* graphics) override;
	bool handleInput(Canvas* canvas, int key, int action) override;
};

#endif
