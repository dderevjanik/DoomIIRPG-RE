#include <span>
#include <cmath>
#include <stdexcept>
#include "Log.h"

#include "SDLGL.h"
#include "CAppContainer.h"
#include "App.h"
#include "Canvas.h"
#include "Graphics.h"
#include "GLES.h"
#include "Render.h"
#include "TinyGL.h"


TinyGL::TinyGL() = default;

TinyGL::~TinyGL() {
}

bool TinyGL::startup(int /*screenWidth*/) {
	this->app = CAppContainer::getInstance()->app;
	Canvas* canvas = this->app->canvas.get();

	LOG_INFO("TinyGL::startup\n");

	// All TinyGL state has migrated to Render (B3.x). The setViewport / setView
	// pair below seeds the camera so the very first DrawSkyMap (called before
	// any gameplay setView) doesn't render through a zero matrix.
	Render* r = this->app->render.get();
	r->setViewport(canvas->viewRect[0], canvas->viewRect[1], canvas->viewRect[2], canvas->viewRect[3]);
	r->setView(0, 0, 0, 0, 0, 0, 290, 290);
	return true;
}
