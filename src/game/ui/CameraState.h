#ifndef __CAMERASTATE_H__
#define __CAMERASTATE_H__

#include "ICanvasState.h"

class CameraState : public ICanvasState {
public:
	void onEnter(Canvas* canvas) override;
	void onExit(Canvas* canvas) override;
	void update(Canvas* canvas) override;
	void render(Canvas* canvas, Graphics* graphics) override;
	bool handleInput(Canvas* canvas, int key, int action) override;
};

#endif
