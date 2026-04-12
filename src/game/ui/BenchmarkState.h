#ifndef __BENCHMARKSTATE_H__
#define __BENCHMARKSTATE_H__

#include "ICanvasState.h"

class BenchmarkState : public ICanvasState {
public:
	void onEnter(Canvas* canvas) override;
	void onExit(Canvas* canvas) override;
	void update(Canvas* canvas) override;
	void render(Canvas* canvas, Graphics* graphics) override;
	bool handleInput(Canvas* canvas, int key, int action) override;
};

#endif
