#include "App.h"
#include "Canvas.h"
#include "Game.h"
#include "Render.h"

// uncoverAutomap stays on Canvas — called from handlePlayingEvents in Canvas.cpp
void Canvas::uncoverAutomap() {

	if (!app->game->updateAutomap) {
		return;
	}
	int n = this->destX >> 6;
	int n2 = this->destY >> 6;
	if (n < 0 || n >= 32 || n2 < 0 || n2 >= 32) {
		return;
	}
	for (int i = n2 - 1; i <= n2 + 1; ++i) {
		if (i >= 0) {
			if (i < 31) {
				for (int j = n - 1; j <= n + 1; ++j) {
					if (j >= 0) {
						if (j < 31) {
							uint8_t b = app->render->mapFlags[i * 32 + j];
							if ((j == n && i == n2) || (b & 0x2) == 0x0) {
								app->render->mapFlags[i * 32 + j] |= (uint8_t)128;
							}
						}
					}
				}
			}
		}
	}
}
