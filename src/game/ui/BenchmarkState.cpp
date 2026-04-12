#include "BenchmarkState.h"
#include "CAppContainer.h"
#include "App.h"
#include "Canvas.h"

void BenchmarkState::onEnter(Canvas* canvas) {
}

void BenchmarkState::onExit(Canvas* canvas) {
}

void BenchmarkState::update(Canvas* canvas) {
	if (canvas->state == Canvas::ST_BENCHMARK) {
		canvas->renderOnlyState();
	}
}

void BenchmarkState::render(Canvas* canvas, Graphics* graphics) {
	// Benchmark overlay rendering is standalone in backPaint (not in handler dispatch)
}

bool BenchmarkState::handleInput(Canvas* canvas, int key, int action) {
	canvas->setState(Canvas::ST_PLAYING);
	canvas->setAnimFrames(canvas->animFrames);
	return true;
}
