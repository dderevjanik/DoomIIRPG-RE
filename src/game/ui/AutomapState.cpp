#include "AutomapState.h"
#include "Canvas.h"
#include "CAppContainer.h"
#include "App.h"
#include "Game.h"
#include "Player.h"
#include "Hud.h"
#include "Render.h"
#include "Graphics.h"
#include "Image.h"
#include "Text.h"
#include "Button.h"
#include "Entity.h"
#include "EntityDef.h"
#include "Enums.h"

static void drawAutomap(Canvas* canvas, Graphics* graphics, bool b) {
	Applet* app = canvas->app;
	graphics->drawRegion(canvas->imgGameHelpBG, 0, 0, Applet::IOS_WIDTH, Applet::IOS_HEIGHT, 0, 0, 0, 0, 0);

	int n3 = 8;
	int n6 = 0x400000 / (n3 << 8);
	int n4 = 112;
	int n5 = 32;

	int n7 = 0;
	for (int i = 0; i < 32; ++i) {
		for (int j = 0; j < 32; ++j) {
			uint8_t b2 = app->render->mapFlags[n7 + j];
			if ((b2 & 0x8) == 0x0) {
				if (app->render->mapEntranceAutomap == (i * 32) + j) {
					graphics->setColor(0xFF00FF00);
					graphics->fillRect(n4 + (n3 * j) + canvas->screenRect[0], (n5 - 1) + (n3 * i) + canvas->screenRect[1], n3, n3);
				}
				else if ((b2 & 0x80) != 0x0) {
					if (app->render->mapExitAutomap == (i * 32) + j) {
						graphics->setColor(0xFFFF0000);
						graphics->fillRect(n4 + (n3 * j) + canvas->screenRect[0], (n5 - 1) + (n3 * i) + canvas->screenRect[1], n3, n3);
					}
					else if ((b2 & 0x1) == 0x0) {
						graphics->setColor(0xFF90B9E7);
						graphics->fillRect(n4 + (n3 * j) + canvas->screenRect[0], (n5 - 1) + (n3 * i) + canvas->screenRect[1] + 1, n3, n3);
						graphics->setColor(0xFF2A3657);
					}
				}
			}
		}
		n7 += 32;
	}

	for (int k = 0; k < Render::MAX_LADDERS_PER_MAP; ++k) {
		if (app->render->mapLadders[k] >= 0 && app->render->mapLadders[k] < 1024 &&
			(app->render->mapFlags[app->render->mapLadders[k]] & 0x80) != 0x0 &&
			(app->render->mapFlags[app->render->mapLadders[k]] & 0x8) == 0x0) {
			int n8 = app->render->mapLadders[k] % 32;
			int n9 = app->render->mapLadders[k] / 32;
			graphics->setColor(0xFFFFFF00);
			graphics->fillRect(n4 + (n3 * n8), n5 + (n3 * n9), n3, n3);
			graphics->setColor(0xFF000000);
			for (int l = 0; l < 8; l += 2) {
				graphics->drawLine(n4 + n3 * n8, n5 + n3 * n9 + l, n4 + n3 * n8 + n3, n5 + n3 * n9 + l);
			}
			graphics->drawLine(n4 + n3 * n8, n5 + n3 * n9, n4 + n3 * n8, n5 + n3 * n9 + n3);
			graphics->drawLine(n4 + n3 * n8 + n3, n5 + n3 * n9, n4 + n3 * n8 + n3, n5 + n3 * n9 + n3);
		}
	}

	graphics->setColor(0xFF2A3657);
	for (int n10 = 0; n10 < app->render->numLines; ++n10) {
		int n11 = app->render->lineFlags[n10 >> 1] >> ((n10 & 0x1) << 2) & 0xF;
		if ((n11 & 0x8) != 0x0) {
			int n12 = n11 & 0x7;
			if (n12 == 6 || n12 == 0) {
				graphics->drawLine(n4 + app->render->lineXs[n10 << 1], n5 + app->render->lineYs[n10 << 1], n4 + app->render->lineXs[(n10 << 1) + 1], n5 + app->render->lineYs[(n10 << 1) + 1]);
			}
		}
	}

	int n13 = 0;
	for (int n14 = 0; n14 < 32; ++n14) {
		for (int n15 = 0; n15 < 32; ++n15) {
			uint8_t b3 = app->render->mapFlags[n13 + n15];
			for (Entity* nextOnTile = app->game->entityDb[n13 + n15]; nextOnTile != nullptr; nextOnTile = nextOnTile->nextOnTile) {
				if (nextOnTile != &app->game->entities[1]) {
					if (nextOnTile != &app->game->entities[0]) {
						if ((b3 & 0x8) == 0x0) {
							int sprite = nextOnTile->getSprite();
							int n16 = app->render->getSpriteInfoRaw(sprite);
							int n17 = (n16 & 0xFF00) >> 8;
							if (0x0 != (n16 & 0x200000)) {
								if (0x0 == (n16 & 0x10000)) {
									int color = 0;
									bool b4 = false;
									bool b5 = false;
									int n18 = 2;
									if (nextOnTile->def->eType == Enums::ET_DOOR) {
										const auto& gc = *canvas->gameConfig;
										short tileIndex = nextOnTile->def->tileIndex;
										color = gc.automapDoorDefault;
										for (const auto& dc : gc.automapDoorColors) {
											if (dc.tileIndex == tileIndex) { color = dc.color; break; }
										}
										b5 = true;
									}
									else if ((n16 & 0x400000) != 0x0) {
										color = 0xFF2A3657;
										b5 = true;
									}
									else if (nextOnTile->def->eType == Enums::ET_NONOBSTRUCTING_SPRITEWALL) {
										color = 0xFF8D8068;
										b5 = true;
									}
									else if (nextOnTile->def->eType == Enums::ET_NPC) {
										b4 = true;
										color = 0xFF0000FF;
										n18 = 128;
									}
									else if (nextOnTile->def->eType == Enums::ET_ATTACK_INTERACTIVE && n17 == 0) {
										color = 0xFF8000FF;
									}
									else if (nextOnTile->def->eType == Enums::ET_MONSTER) {
										color = 0xFFFF8000;
										n18 = 128;
									}
									else if (nextOnTile->def->eType == Enums::ET_DECOR) {
										color = 0xFF8D8068;
										const auto& hiddenDecors = canvas->gameConfig->automapHiddenDecors;
										for (int16_t hd : hiddenDecors) {
											if (nextOnTile->def->tileIndex == hd) { color = 0; break; }
										}
									}
									else if (app->player->god && nextOnTile->def->eType == Enums::ET_ITEM && nextOnTile->def->eSubType != Enums::ITEM_KEY) {
										color = 0xFF00FFEA;
									}
									if (color != 0 && ((b3 & n18) != 0x0 || (n18 & 0x80) == 0x0)) {
										graphics->setColor(color);
										if ((n16 & 0xF000000) != 0x0) {
											int n20;
											int n19 = n20 = app->render->getSpriteX(sprite);
											int n22;
											int n21 = n22 = app->render->getSpriteY(sprite);
											if ((n16 & 0x3000000) != 0x0) {
												n20 -= 32;
												n19 += 32;
											}
											else {
												n22 -= 32;
												n21 += 32;
											}
											int n23 = ((n20 << 16) / n6) + 1 + 128 >> 8;
											int n24 = ((n19 << 16) / n6) + 128 >> 8;
											int n25 = ((n22 << 16) / n6) + 128 >> 8;
											int n26 = ((n21 << 16) / n6) + 128 >> 8;
											if (b5) {
												graphics->drawLine(n4 + n23, n5 + n25, n4 + n24, n5 + n26);
											}
											else {
												graphics->fillRect(n4 + (n23 + n24 >> 1) - (n3 / 4), n5 + (n25 + n26 >> 1) - (n3 / 4), n3 / 2, n3 / 2);
											}
										}
										else if (b4) {
											graphics->fillRect(n4 + (n3 * n15) + (n3 / 4), n5 + (n3 * n14) + (n3 / 4), (n3 / 2) + 2, (n3 / 2) + 2);
										}
										else {
											graphics->fillRect(n4 + (n3 * n15) + (n3 / 4) + 1, n5 + (n3 * n14) + (n3 / 4) + 1, (n3 / 2), (n3 / 2));
										}
									}
								}
							}
						}
					}
				}
			}
		}
		n13 += 32;
	}

	for (int n30 = 0; n30 < app->player->numNotebookIndexes; ++n30) {
		if (!app->player->isQuestDone(n30)) {
			if (!app->player->isQuestFailed(n30)) {
				int n31 = app->player->notebookPositions[n30] >> 5 & 0x1F;
				int n32 = app->player->notebookPositions[n30] & 0x1F;
				if (n31 + n32 != 0 && (0x80 & app->render->mapFlags[n32 * 32 + n31]) != 0x0) {
					graphics->setColor(((app->time / 1024 & 0x1) == 0x0) ? 0xFFFF0000 : 0xFF00FF00);
					Entity* mapEntity = app->game->findMapEntity(n31 << 6, n32 << 6, 32);
					if (nullptr != mapEntity) {
						int sprite2 = mapEntity->getSprite();
						int n33 = app->render->getSpriteInfoRaw(sprite2);
						int n35;
						int n34 = n35 = app->render->getSpriteX(sprite2);
						int n37;
						int n36 = n37 = app->render->getSpriteY(sprite2);
						if ((n33 & 0x3000000) != 0x0) {
							n35 -= 32;
							n34 += 32;
						}
						else {
							n37 -= 32;
							n36 += 32;
						}
						graphics->drawLine(n4 + ((n35 << 16) / n6 + 1 + 128 >> 8), n5 + ((n37 << 16) / n6 + 128 >> 8), n4 + ((n34 << 16) / n6 + 128 >> 8), n5 + ((n36 << 16) / n6 + 128 >> 8));
					}
					else {
						graphics->fillRect(n4 + n3 * n31 + n3 / 4, n5 + n3 * n32 + n3 / 4, n3 / 2 + 2, n3 / 2 + 2);
					}
				}
			}
		}
	}

	if (app->time > canvas->automapBlinkTime) {
		canvas->automapBlinkTime = app->time + 333;
		canvas->automapBlinkState = !canvas->automapBlinkState;
	}

	int n38 = 0;
	switch (canvas->destAngle & 0x3FF) {
	case Enums::ANGLE_NORTH: n38 = 0; break;
	case Enums::ANGLE_SOUTH: n38 = 1; break;
	case Enums::ANGLE_EAST: n38 = 2; break;
	case Enums::ANGLE_WEST: n38 = 3; break;
	}
	int n39 = n38 * 4;
	if (canvas->automapBlinkState) {
		n39 += 16;
	}
	int n40 = n4 + ((n3 * (canvas->viewX - 32)) / 64) + (n3 / 2);
	int n41 = n5 + ((n3 * (canvas->viewY - 32)) / 64) + (n3 / 2);
	if (n41 < canvas->screenRect[1] + canvas->screenRect[3]) {
		graphics->drawRegion(canvas->imgMapCursor, 0, n39, 4, 4, n40, n41, 3, 0, 0);
	}

	Text* LargeBuffer = app->localization->getLargeBuffer();
	LargeBuffer->setLength(0);
	app->localization->composeText(canvas->softKeyLeftID, LargeBuffer);
	LargeBuffer->dehyphenate();
	app->setFontRenderMode(2);
	if (canvas->m_softKeyButtons->GetButton(19)->highlighted) {
		app->setFontRenderMode(0);
	}
	graphics->drawString(LargeBuffer, 15, 310, 36);
	app->setFontRenderMode(0);

	LargeBuffer->setLength(0);
	app->localization->composeText(canvas->softKeyRightID, LargeBuffer);
	LargeBuffer->dehyphenate();
	app->setFontRenderMode(2);
	if (canvas->m_softKeyButtons->GetButton(20)->highlighted) {
		app->setFontRenderMode(0);
	}
	graphics->drawString(LargeBuffer, 465, 310, 40);
	app->setFontRenderMode(0);
	LargeBuffer->dispose();
}

void AutomapState::onEnter(Canvas* canvas) {
	Applet* app = canvas->app;
	canvas->automapDrawn = false;
	canvas->automapTime = app->time;
}

void AutomapState::onExit(Canvas* canvas) {
	Applet* app = canvas->app;
	app->player->unpause(app->time - canvas->automapTime);
}

void AutomapState::update(Canvas* canvas) {
	Applet* app = canvas->app;
	app->game->updateLerpSprites();
	if (!canvas->automapDrawn && app->game->animatingEffects == 0) {
		canvas->updateView();
		canvas->repaintFlags &= ~Canvas::REPAINT_VIEW3D;
		if (canvas->state != Canvas::ST_AUTOMAP) {
			canvas->updateView();
		}
	}
	if (canvas->state == Canvas::ST_AUTOMAP || canvas->state == Canvas::ST_PLAYING) {
		canvas->drawPlayingSoftKeys();
	}
}

void AutomapState::render(Canvas* canvas, Graphics* graphics) {
	Applet* app = canvas->app;
	drawAutomap(canvas, graphics, !canvas->automapDrawn);
	canvas->m_softKeyButtons->Render(graphics);
	app->hud->drawArrowControls(graphics);
	canvas->automapDrawn = true;
}

bool AutomapState::handleInput(Canvas* canvas, int key, int action) {
	if (key == 18) {
		return true;
	}
	canvas->automapDrawn = false;
	return canvas->handlePlayingEvents(key, action);
}
