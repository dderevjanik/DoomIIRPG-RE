#ifndef __TRAVELMAPSTATE_H__
#define __TRAVELMAPSTATE_H__

#include "ICanvasState.h"

class TravelMapState : public ICanvasState {
public:
	void onEnter(Canvas* canvas) override;
	void onExit(Canvas* canvas) override;
	void update(Canvas* canvas) override;
	void render(Canvas* canvas, Graphics* graphics) override;
	bool handleInput(Canvas* canvas, int key, int action) override;
};

#endif
