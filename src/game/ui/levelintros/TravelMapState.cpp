#include "TravelMapState.h"

#include <cmath>
#include <numbers>

#include "CAppContainer.h"
#include "App.h"
#include "Canvas.h"
#include "Game.h"
#include "Player.h"
#include "Render.h"
#include "Graphics.h"
#include "Image.h"
#include "Text.h"
#include "Button.h"
#include "Sound.h"
#include "Sounds.h"
#include "SoundNames.h"
#include "Enums.h"
#include "Menus.h"
#include "DataNode.h"
#include "SpriteDefs.h"

#include <array>
#include <map>

// --- Travel map data structures (local to this file) ---

struct TravelMapPlanet {
	std::string name;
	int origin[2] = {0, 0};
	std::string closeupImage;
	int nameStringId = 0;
	std::vector<std::array<int, 2>> nameLabels;
	std::string arrivalCloseupImage;
	int arrivalOrigin[2] = {0, 0};
	bool hasArrivalCloseup = false;
	int specialMarker[2] = {0, 0};
	bool hasSpecialMarker = false;
};

struct TravelMapPath {
	std::string from, to, image;
	int position[2] = {0, 0};
	float nameRevealFraction = 0.5f;
};

struct TravelMapTiming {
	int samePlanetCloseupDelay = 1500;
	int crossPlanetCloseupDelay = 700;
	int dottedLineDivisor = 22;
	int locatorSpeedSamePlanet = 15;
	int locatorSpeedCrossPlanet = 7;
	int finalHold = 800;
	int dottedLinePause = 500;
	int gridScrollPeriod = 200;
	int gridCellSize = 44;
};

struct TravelMapImages {
	std::string background = "TravelMap.bmp";
	std::string horzGrid = "travelMapHorzGrid.bmp";
	std::string vertGrid = "travelMapVertGrid.bmp";
	std::string highlight = "highlight.bmp";
	std::string magnifyingGlass = "magnifyingGlass.bmp";
	std::string spaceship = "spaceShip.bmp";
};

struct TravelMapLevelData {
	std::string planet;
	int crosshair[2] = {0, 0};
	int locatorBox[2] = {0, 0};
};

struct TravelMapConfig {
	TravelMapImages images;
	TravelMapTiming timing;
	std::map<std::string, TravelMapPlanet> planets;
	std::vector<TravelMapPath> paths;
	TravelMapLevelData loadLevel;
	TravelMapLevelData lastLevel;
	bool loaded = false;
};

// --- Module-local config loaded at runtime from travel_map.yaml ---
static TravelMapConfig s_tmConfig;

// Forward declarations for static helpers
static void drawStarFieldPage(Canvas* canvas, Graphics* graphics);
static void drawStarField(Canvas* canvas, Graphics* graphics, int x, int y);
static void runStarFieldFrame(Canvas* canvas);
static void drawGridLines(Canvas* canvas, Graphics* graphics, int i);
static void drawAppropriateCloseup(Canvas* canvas, Graphics* graphics, int n, bool b);
static void drawLocatorBoxAndName(Canvas* canvas, Graphics* graphics, bool b, int n, Text* text);
static bool drawDottedLine(Canvas* canvas, Graphics* graphics, int n);
static bool drawDottedLine(Canvas* canvas, Graphics* graphics);
static bool drawStartPathLine(Canvas* canvas, Graphics* graphics, int n, const TravelMapPath* path);
static int yCoordOfSpaceShip(Canvas* canvas, int n);
static bool drawPlanetPathLine(Canvas* canvas, Graphics* graphics, int n, const TravelMapPath* path, bool reversed);
static bool drawLocatorLines(Canvas* canvas, Graphics* graphics, int n, bool b, bool b2);
static bool newLevelSamePlanet(Canvas* canvas);
static void finishTravelMapAndLoadLevel(Canvas* canvas);
static void disposeTravelMap(Canvas* canvas);
static void initTravelMap(Canvas* canvas);
static void handleTravelMapInput(Canvas* canvas, int key, int action);
static void drawTravelMap(Canvas* canvas, Graphics* graphics);

// --- Parse a travel_map.yaml node into per-level data ---
static TravelMapLevelData parseLevelData(const DataNode& node) {
	TravelMapLevelData ld;
	ld.planet = node["planet"].asString("");
	DataNode ch = node["crosshair"];
	if (ch && ch.isSequence() && ch.size() >= 2) {
		ld.crosshair[0] = ch[0].asInt(0);
		ld.crosshair[1] = ch[1].asInt(0);
	}
	DataNode lb = node["locator_box"];
	if (lb && lb.isSequence() && lb.size() >= 2) {
		ld.locatorBox[0] = lb[0].asInt(0);
		ld.locatorBox[1] = lb[1].asInt(0);
	}
	return ld;
}

// --- Parse a travel_map.yaml into TravelMapConfig ---
static void parseTravelMapYaml(const DataNode& node, TravelMapConfig& cfg) {
	cfg.loaded = true;

	DataNode imgs = node["images"];
	if (imgs) {
		cfg.images.background = imgs["background"].asString(cfg.images.background);
		cfg.images.horzGrid = imgs["horz_grid"].asString(cfg.images.horzGrid);
		cfg.images.vertGrid = imgs["vert_grid"].asString(cfg.images.vertGrid);
		cfg.images.highlight = imgs["highlight"].asString(cfg.images.highlight);
		cfg.images.magnifyingGlass = imgs["magnifying_glass"].asString(cfg.images.magnifyingGlass);
		cfg.images.spaceship = imgs["spaceship"].asString(cfg.images.spaceship);
	}

	DataNode timing = node["timing"];
	if (timing) {
		auto& t = cfg.timing;
		t.samePlanetCloseupDelay = timing["same_planet_closeup_delay"].asInt(t.samePlanetCloseupDelay);
		t.crossPlanetCloseupDelay = timing["cross_planet_closeup_delay"].asInt(t.crossPlanetCloseupDelay);
		t.dottedLineDivisor = timing["dotted_line_divisor"].asInt(t.dottedLineDivisor);
		t.locatorSpeedSamePlanet = timing["locator_speed_same_planet"].asInt(t.locatorSpeedSamePlanet);
		t.locatorSpeedCrossPlanet = timing["locator_speed_cross_planet"].asInt(t.locatorSpeedCrossPlanet);
		t.finalHold = timing["final_hold"].asInt(t.finalHold);
		t.dottedLinePause = timing["dotted_line_pause"].asInt(t.dottedLinePause);
		t.gridScrollPeriod = timing["grid_scroll_period"].asInt(t.gridScrollPeriod);
		t.gridCellSize = timing["grid_cell_size"].asInt(t.gridCellSize);
	}

	DataNode planets = node["planets"];
	if (planets && planets.isMap()) {
		for (auto it = planets.begin(); it != planets.end(); ++it) {
			std::string pName = it.key().asString();
			DataNode pNode = it.value();
			TravelMapPlanet planet;
			planet.name = pName;
			DataNode orig = pNode["origin"];
			if (orig && orig.isSequence() && orig.size() >= 2) {
				planet.origin[0] = orig[0].asInt(0);
				planet.origin[1] = orig[1].asInt(0);
			}
			planet.closeupImage = pNode["closeup_image"].asString("");
			planet.nameStringId = pNode["name_string_id"].asInt(0);
			DataNode nls = pNode["name_labels"];
			if (nls && nls.isSequence()) {
				for (auto it2 = nls.begin(); it2 != nls.end(); ++it2) {
					DataNode lbl = it2.value();
					if (lbl.isSequence() && lbl.size() >= 2) {
						planet.nameLabels.push_back({lbl[0].asInt(0), lbl[1].asInt(0)});
					}
				}
			}
			DataNode ac = pNode["arrival_closeup"];
			if (ac) {
				planet.hasArrivalCloseup = true;
				planet.arrivalCloseupImage = ac["image"].asString("");
				DataNode ao = ac["origin"];
				if (ao && ao.isSequence() && ao.size() >= 2) {
					planet.arrivalOrigin[0] = ao[0].asInt(0);
					planet.arrivalOrigin[1] = ao[1].asInt(0);
				}
			}
			DataNode sm = pNode["special_marker"];
			if (sm && sm.isSequence() && sm.size() >= 2) {
				planet.hasSpecialMarker = true;
				planet.specialMarker[0] = sm[0].asInt(0);
				planet.specialMarker[1] = sm[1].asInt(0);
			}
			cfg.planets[pName] = std::move(planet);
		}
	}

	DataNode paths = node["paths"];
	if (paths && paths.isSequence()) {
		for (auto it = paths.begin(); it != paths.end(); ++it) {
			DataNode pNode = it.value();
			TravelMapPath path;
			path.from = pNode["from"].asString("");
			path.to = pNode["to"].asString("");
			path.image = pNode["image"].asString("");
			DataNode pos = pNode["position"];
			if (pos && pos.isSequence() && pos.size() >= 2) {
				path.position[0] = pos[0].asInt(0);
				path.position[1] = pos[1].asInt(0);
			}
			path.nameRevealFraction = pNode["name_reveal_fraction"].asFloat(path.nameRevealFraction);
			cfg.paths.push_back(std::move(path));
		}
	}
}

// --- Load travel map config from per-level YAML files ---
static void loadTravelMapConfig(Canvas* canvas) {
	const auto& gc = CAppContainer::getInstance()->gameConfig;
	s_tmConfig = TravelMapConfig{}; // reset

	// Load the destination level's travel_map.yaml
	auto it = gc.levelInfos.find((int)canvas->TM_LoadLevelId);
	if (it == gc.levelInfos.end() || it->second.introFile.empty()) {
		printf("[travel_map] No intro config for load level %d (introFile='%s')\n",
			canvas->TM_LoadLevelId,
			(it != gc.levelInfos.end()) ? it->second.introFile.c_str() : "N/A");
		return;
	}

	std::string yamlPath = it->second.dir + "/" + it->second.introFile;
	DataNode node = DataNode::loadFile(yamlPath.c_str());
	if (!node) {
		printf("[travel_map] Failed to load: %s\n", yamlPath.c_str());
		return;
	}

	parseTravelMapYaml(node, s_tmConfig);
	s_tmConfig.loadLevel = parseLevelData(node);
	printf("[travel_map] Loaded %s: planet=%s, %zu planets, %zu paths\n",
		yamlPath.c_str(), s_tmConfig.loadLevel.planet.c_str(),
		s_tmConfig.planets.size(), s_tmConfig.paths.size());

	// Also load the previous level's travel_map.yaml for its planet/crosshair
	if (canvas->TM_LastLevelId > 0) {
		auto it2 = gc.levelInfos.find((int)canvas->TM_LastLevelId);
		if (it2 != gc.levelInfos.end() && !it2->second.introFile.empty()) {
			std::string lastPath = it2->second.dir + "/" + it2->second.introFile;
			DataNode lastNode = DataNode::loadFile(lastPath.c_str());
			if (lastNode) {
				s_tmConfig.lastLevel = parseLevelData(lastNode);
				printf("[travel_map] Last level %d: planet=%s\n",
					canvas->TM_LastLevelId, s_tmConfig.lastLevel.planet.c_str());
			} else {
				printf("[travel_map] Failed to load last level: %s\n", lastPath.c_str());
			}
		} else {
			printf("[travel_map] No intro config for last level %d\n", canvas->TM_LastLevelId);
		}
	}
}

// --- Data-driven travel map helpers ---

static const std::string& getPlanetForLevel(int levelId) {
	static const std::string empty;
	static const std::string start = "start";
	if (levelId == 0) return start;
	// Check if it's the load or last level (the two we have data for)
	const auto& gc = CAppContainer::getInstance()->gameConfig;
	auto it = gc.levelInfos.find(levelId);
	if (it == gc.levelInfos.end()) return empty;
	// Determine which level data to use
	if (levelId == CAppContainer::getInstance()->app->canvas->TM_LoadLevelId) {
		return s_tmConfig.loadLevel.planet;
	}
	return s_tmConfig.lastLevel.planet;
}

static const TravelMapLevelData& getLevelData(int levelId) {
	Canvas* canvas = CAppContainer::getInstance()->app->canvas.get();
	if (levelId == canvas->TM_LoadLevelId) return s_tmConfig.loadLevel;
	return s_tmConfig.lastLevel;
}

static const TravelMapPlanet* getPlanetConfig(const std::string& name) {
	auto it = s_tmConfig.planets.find(name);
	if (it != s_tmConfig.planets.end()) return &it->second;
	return nullptr;
}

static const TravelMapPath* findPath(const std::string& fromPlanet, const std::string& toPlanet) {
	for (const auto& path : s_tmConfig.paths) {
		if ((path.from == fromPlanet && path.to == toPlanet) ||
			(path.from == toPlanet && path.to == fromPlanet)) {
			return &path;
		}
	}
	return nullptr;
}


static void drawTravelMap(Canvas* canvas, Graphics* graphics) {


	if (canvas->stateVars[5] == 1) {
		drawStarFieldPage(canvas, graphics);
		canvas->staleView = true;
		return;
	}

	graphics->drawImage(canvas->imgTravelBG, canvas->SCR_CX, (canvas->displayRect[3] - canvas->imgTravelBG->height) / 2, 17, 0, 0);

	if (canvas->xDiff > 1 || canvas->yDiff > 1) {
		graphics->fillRegion(canvas->imgFabricBG, 0, 0, canvas->displayRect[2], canvas->yDiff);
		graphics->fillRegion(canvas->imgFabricBG, 0, canvas->yDiff, canvas->xDiff, canvas->displayRect[3] - canvas->yDiff);
		graphics->fillRegion(canvas->imgFabricBG, canvas->xDiff, canvas->yDiff + canvas->mapHeight, canvas->mapWidth, canvas->yDiff);
		graphics->fillRegion(canvas->imgFabricBG, canvas->xDiff + canvas->mapWidth, canvas->yDiff, canvas->xDiff, canvas->displayRect[3] - canvas->yDiff);
		graphics->drawRect(canvas->xDiff + -1, canvas->yDiff + -1, canvas->mapWidth + 1, canvas->mapHeight + 1, 0xFF000000);
		graphics->clipRect(canvas->xDiff, canvas->yDiff, canvas->imgTravelBG->width, canvas->imgTravelBG->height);
	}

	const auto& t = s_tmConfig.timing;
	int time = canvas->app->upTimeMs - canvas->stateVars[0];
	drawGridLines(canvas, graphics, time + canvas->totalTMTimeInPastAnimations);

	bool levelSamePlanet = newLevelSamePlanet(canvas);
	if (canvas->stateVars[1] != 1) {
		if (time > (levelSamePlanet ? t.samePlanetCloseupDelay : t.crossPlanetCloseupDelay)) {
			canvas->totalTMTimeInPastAnimations += time;
			canvas->stateVars[0] += time;
			canvas->stateVars[1] = 1;
			time = 0;
			if (levelSamePlanet) {
				canvas->stateVars[2] = 1;
				canvas->stateVars[3] = 1;
			}
		}
		else if (levelSamePlanet) {
			drawAppropriateCloseup(canvas, graphics, canvas->TM_LastLevelId, false);
			Text* smallBuffer = canvas->app->localization->getSmallBuffer();
			smallBuffer->setLength(0);
			canvas->app->localization->composeText((short)3, canvas->app->game->levelNames[canvas->TM_LastLevelId - 1], smallBuffer);
			smallBuffer->dehyphenate();
			drawLocatorBoxAndName(canvas, graphics, (time & 0x200) == 0x0, canvas->TM_LastLevelId, smallBuffer);
			smallBuffer->dispose();
		}
	}

	if (canvas->stateVars[1] == 1) {
		if (canvas->stateVars[2] != 1) {
			if (drawDottedLine(canvas, graphics, time)) {
				canvas->totalTMTimeInPastAnimations += time;
				canvas->stateVars[0] += time;
				canvas->stateVars[2] = 1;
			}
		}
		else if (canvas->stateVars[2] == 1) {
			if (canvas->stateVars[3] != 1) {
				drawDottedLine(canvas, graphics);
				if (time > t.dottedLinePause || levelSamePlanet) {
					canvas->totalTMTimeInPastAnimations += time;
					canvas->stateVars[0] += time;
					canvas->stateVars[3] = 1;
				}
			}
			else if (canvas->stateVars[3] == 1) {
				drawAppropriateCloseup(canvas, graphics, canvas->TM_LoadLevelId, canvas->stateVars[8] == 1);
				if (canvas->stateVars[6] == 1) {
					Text* smallBuffer2 = canvas->app->localization->getSmallBuffer();
					smallBuffer2->setLength(0);
					canvas->app->localization->composeText((short)3, canvas->app->game->levelNames[canvas->TM_LoadLevelId - 1], smallBuffer2);
					smallBuffer2->dehyphenate();

					drawLocatorBoxAndName(canvas, graphics, (time & 0x200) == 0x0, canvas->TM_LoadLevelId, smallBuffer2);
					smallBuffer2->setLength(0);
					canvas->app->localization->composeText((short)0, (short)96, smallBuffer2);
					smallBuffer2->wrapText(24);
					smallBuffer2->dehyphenate();
					graphics->drawString(smallBuffer2, canvas->SCR_CX, canvas->displayRect[3] - 21, 17);
					smallBuffer2->dispose();
				}
				else if (canvas->stateVars[4] != 1) {
					if (drawLocatorLines(canvas, graphics, time, levelSamePlanet, canvas->stateVars[8] == 1)) {
						canvas->totalTMTimeInPastAnimations += time;
						canvas->stateVars[0] += time;
						canvas->stateVars[4] = 1;
					}
				}
				else {
					drawLocatorLines(canvas, graphics, -1, levelSamePlanet, canvas->stateVars[8] == 1);
					if (time > t.finalHold) {
						canvas->totalTMTimeInPastAnimations += time;
						canvas->stateVars[0] += time;
						if (canvas->stateVars[8] == 0) {
							canvas->stateVars[6] = 1;
						}
						else {
							canvas->stateVars[4] = 0;
							canvas->stateVars[8] = 0;
							const auto* planet = getPlanetConfig(getPlanetForLevel(canvas->TM_LoadLevelId));
							const auto& ld = getLevelData(canvas->TM_LoadLevelId);
							if (planet) {
								canvas->targetX = ld.crosshair[0] + canvas->xDiff + planet->origin[0];
								canvas->targetY = ld.crosshair[1] + canvas->yDiff + planet->origin[1];
							}
						}
					}
				}
			}
		}
	}

	canvas->staleView = true;
}


static bool newLevelSamePlanet(Canvas* canvas) {
	if (canvas->TM_LastLevelId == canvas->TM_LoadLevelId) return false;
	const auto& pa = getPlanetForLevel(canvas->TM_LastLevelId);
	const auto& pb = getPlanetForLevel(canvas->TM_LoadLevelId);
	return !pa.empty() && !pb.empty() && pa == pb;
}


static void drawAppropriateCloseup(Canvas* canvas, Graphics* graphics, int n, bool b) {
	const auto* planet = getPlanetConfig(getPlanetForLevel(n));
	if (!planet) return;

	if (planet->hasArrivalCloseup && b) {
		graphics->drawImage(canvas->imgEarthCloseUp, planet->origin[0], planet->origin[1], 0, 0, 0);
	} else if (planet->hasArrivalCloseup && !b) {
		graphics->drawImage(canvas->imgTierCloseUp, planet->arrivalOrigin[0], planet->arrivalOrigin[1], 0, 0, 0);
	} else {
		graphics->drawImage(canvas->imgTierCloseUp, planet->origin[0], planet->origin[1], 0, 0, 0);
	}
}


static bool drawDottedLine(Canvas* canvas, Graphics* graphics, int n) {
	const auto& t = s_tmConfig.timing;
	int n2 = n / t.dottedLineDivisor;

	const auto& fromPlanet = getPlanetForLevel(canvas->TM_LastLevelId);
	const auto& toPlanet = getPlanetForLevel(canvas->TM_LoadLevelId);

	const auto* path = findPath(fromPlanet, toPlanet);
	if (!path) return true;

	bool reversed = (path->from != fromPlanet);

	if (path->from == "start") {
		return drawStartPathLine(canvas, graphics, n2, path);
	}

	return drawPlanetPathLine(canvas, graphics, n2, path, reversed);
}


static bool drawDottedLine(Canvas* canvas, Graphics* graphics) {
	const auto& t = s_tmConfig.timing;
	return drawDottedLine(canvas, graphics, t.dottedLineDivisor * std::max(canvas->screenRect[2], canvas->screenRect[3]));
}


static bool drawStartPathLine(Canvas* canvas, Graphics* graphics, int n, const TravelMapPath* path) {
	const auto* destPlanet = getPlanetConfig(path->to);
	if (!destPlanet) return true;

	bool b = false;
	int width = canvas->imgTravelPath->width;
	int n2 = n;
	if (n2 > width) {
		b = true;
		n2 = width;
	}
	if (n > (int)(width * path->nameRevealFraction)) {
		Text* smallBuffer = canvas->app->localization->getSmallBuffer();
		smallBuffer->setLength(0);
		canvas->app->localization->composeText((short)3, (short)destPlanet->nameStringId, smallBuffer);
		smallBuffer->dehyphenate();
		if (!destPlanet->nameLabels.empty()) {
			graphics->drawString(smallBuffer, destPlanet->nameLabels[0][0], destPlanet->nameLabels[0][1], 4);
		}
		smallBuffer->dispose();
	}
	graphics->drawRegion(canvas->imgTravelPath, width - n2, 0, n2, canvas->imgTravelPath->height, path->position[0] + width - n2, path->position[1], 0, 0, 0);
	int n3 = width - n2;
	graphics->drawImage(canvas->imgSpaceShip, path->position[0] + n3 - canvas->imgSpaceShip->width, yCoordOfSpaceShip(canvas, n3) + path->position[1] - (canvas->imgSpaceShip->height >> 1), 0, 0, 0);
	return b;
}


static int yCoordOfSpaceShip(Canvas* canvas, int n) {
	int scaled;
	scaled = (n * 11) / 15;
	return ((-851 * (scaled * scaled) / 110880 + 6115 * scaled / 22176 + 40) * 15) / 11;
}


static bool drawPlanetPathLine(Canvas* canvas, Graphics* graphics, int n, const TravelMapPath* path, bool reversed) {
	// Destination is the planet we're traveling TO
	const std::string& destPlanetName = reversed ? path->from : path->to;
	const auto* destPlanet = getPlanetConfig(destPlanetName);

	bool b2 = false;
	int height = canvas->imgTravelPath->height;
	int n2 = n;
	if (n2 > height) {
		b2 = true;
		n2 = height;
	}
	int n3 = reversed ? 0 : (height - n2);
	int width = canvas->imgTravelPath->width;

	if (n > (int)(height * path->nameRevealFraction)) {
		if (destPlanet) {
			Text* smallBuffer = canvas->app->localization->getSmallBuffer();
			smallBuffer->setLength(0);
			canvas->app->localization->composeText((short)3, (short)destPlanet->nameStringId, smallBuffer);
			smallBuffer->dehyphenate();
			int nameLabelIdx = reversed ? 1 : 0;
			if (nameLabelIdx < (int)destPlanet->nameLabels.size()) {
				graphics->drawString(smallBuffer, destPlanet->nameLabels[nameLabelIdx][0], destPlanet->nameLabels[nameLabelIdx][1], 4);
			}
			smallBuffer->dispose();
		}
	}
	graphics->drawRegion(canvas->imgTravelPath, 0, n3, width, n2, path->position[0], path->position[1] + n3, 0, 0, 0);
	return b2;
}


static void drawLocatorBoxAndName(Canvas* canvas, Graphics* graphics, bool b, int n, Text* text) {
	const auto* planet = getPlanetConfig(getPlanetForLevel(n));
	if (!planet) return;
	const auto& ld = getLevelData(n);

	int n2 = planet->origin[0];
	int n3 = planet->origin[1];
	int n5 = ld.crosshair[0] + canvas->xDiff + n2;
	int n6 = ld.crosshair[1] + canvas->yDiff + n3;
	if (b) {
		graphics->drawImage(canvas->imgMagGlass, n5, n6, 3, 0, 0);
	}

	int x = ld.locatorBox[0] + canvas->xDiff + n2;
	int y = ld.locatorBox[1] + canvas->yDiff + n3 + (canvas->imgNameHighlight->height >> 1);
	graphics->drawImage(canvas->imgNameHighlight, x, y, 6, 0, 0);
	canvas->graphics.currentCharColor = 3;
	graphics->drawString(text, x + (canvas->imgNameHighlight->width >> 1), y, 3);
}


static void drawGridLines(Canvas* canvas, Graphics* graphics, int i) {
	const auto& t = s_tmConfig.timing;
	int pos;

	for (pos = (i / t.gridScrollPeriod) % t.gridCellSize + canvas->displayRect[0]; pos < canvas->displayRect[2]; pos += t.gridCellSize) {
		graphics->drawImage(canvas->imgMapVertGridLines, pos, canvas->displayRect[1], 0x14, 0, 2);
	}
	for (pos = canvas->displayRect[1] + 5; pos < canvas->displayRect[3]; pos += t.gridCellSize) {
		graphics->drawImage(canvas->imgMapHorzGridLines, canvas->displayRect[0], pos, 20, 0, 2);
		graphics->drawImage(canvas->imgMapHorzGridLines, 240, pos, 20, 0, 2);
	}
}








static void handleTravelMapInput(Canvas* canvas, int key, int action) {
#if 0 // IOS
	if ((canvas->stateVars[6] == 1) || (key == 18) || (action == Enums::ACTION_FIRE)) {
		finishTravelMapAndLoadLevel(canvas);
	}
	else {
		canvas->stateVars[6] = 1;
	}
#else // J2ME/BREW

	bool hasSavedState = canvas->app->game->hasSavedState();

	if (action == Enums::ACTION_MENU) { // [GEC] skip all
		finishTravelMapAndLoadLevel(canvas);
		return;
	}

	if (action == Enums::ACTION_FIRE) {
		if (canvas->stateVars[6] == 1) {
#if 0 // [GEC] no esta disponible en ningina de las versiones del juego
			if (canvas->TM_LastLevelId == 0 && onMoon(canvas->TM_LoadLevelId)) {
				canvas->stateVars[5] = 1; // Draw start field
			}
			else {
				finishTravelMapAndLoadLevel(canvas);
			}
#else
			finishTravelMapAndLoadLevel(canvas);
#endif
		}
		else if (hasSavedState) {
			if (canvas->stateVars[2] == 1) {
				if (canvas->stateVars[8] == 0) {
					canvas->stateVars[3] = 1;
					canvas->stateVars[4] = 1;
					canvas->stateVars[6] = 1;
				}
				else {
					int n3 = canvas->app->upTimeMs - canvas->stateVars[0];
					canvas->totalTMTimeInPastAnimations += n3;
					canvas->stateVars[0] += n3;
					if (canvas->stateVars[4] == 1) {
						canvas->stateVars[4] = 0;
						canvas->stateVars[8] = 0;
						const auto* planet = getPlanetConfig(getPlanetForLevel(canvas->TM_LoadLevelId));
						const auto& ld = getLevelData(canvas->TM_LoadLevelId);
						if (planet) {
							canvas->targetX = ld.crosshair[0] + canvas->xDiff + planet->origin[0];
							canvas->targetY = ld.crosshair[1] + canvas->yDiff + planet->origin[1];
						}
					}
					else {
						canvas->stateVars[3] = 1;
						canvas->stateVars[4] = 1;
					}
				}
			}
			else {
				canvas->totalTMTimeInPastAnimations += canvas->app->upTimeMs - canvas->stateVars[0];
				canvas->stateVars[0] = canvas->app->upTimeMs;
				canvas->stateVars[1] = 1;
				canvas->stateVars[2] = 1;
				if (newLevelSamePlanet(canvas)) {
					canvas->stateVars[3] = 1;
				}
			}
		}
	}
	else if (action == Enums::ACTION_AUTOMAP && canvas->stateVars[5] == 1) {
		finishTravelMapAndLoadLevel(canvas);
	}
#endif
}

static void finishTravelMapAndLoadLevel(Canvas* canvas) {
	canvas->clearSoftKeys();
	disposeTravelMap(canvas);
	canvas->setLoadingBarText((short)0, (short)41);
	canvas->setState(Canvas::ST_LOADING);
}


static bool drawLocatorLines(Canvas* canvas, Graphics* graphics, int n, bool b, bool b2) {
	const auto& t = s_tmConfig.timing;
	int targetX;
	int targetY;
	int targetX2;
	int targetY2;
	if (n >= 0) {
		int n2 = n / (b ? t.locatorSpeedSamePlanet : t.locatorSpeedCrossPlanet);
		if (b) {
			const auto* planet = getPlanetConfig(getPlanetForLevel(canvas->TM_LastLevelId));
			if (!planet) return true;
			int n3, n4;
			if (planet->hasArrivalCloseup && b2) {
				n3 = planet->arrivalOrigin[0];
				n4 = planet->arrivalOrigin[1];
			} else {
				n3 = planet->origin[0];
				n4 = planet->origin[1];
			}
			const auto& ld = getLevelData(canvas->TM_LastLevelId);
			targetX = ld.crosshair[0] + canvas->xDiff + n3;
			targetY = ld.crosshair[1] + canvas->yDiff + n4;
		}
		else {
			targetX = canvas->displayRect[2] - canvas->xDiff;
			targetY = canvas->displayRect[3] - canvas->yDiff;
		}
		int n6 = (canvas->targetX > targetX) ? 1 : ((canvas->targetX == targetX) ? 0 : -1);
		int n7 = (canvas->targetY > targetY) ? 1 : ((canvas->targetY == targetY) ? 0 : -1);
		targetX2 = targetX + n6 * n2;
		targetY2 = targetY + n7 * n2;
	}
	else {
		targetX2 = (targetX = canvas->targetX);
		targetY2 = (targetY = canvas->targetY);
	}
	bool b3 = false;
	bool b4 = false;
	if ((canvas->targetX - targetX) * (canvas->targetX - targetX2) < 0 || canvas->targetX == targetX2) {
		targetX2 = canvas->targetX;
		b3 = true;
	}
	if ((canvas->targetY - targetY) * (canvas->targetY - targetY2) < 0 || canvas->targetY == targetY2) {
		targetY2 = canvas->targetY;
		b4 = true;
	}
	graphics->drawLine(targetX2 + 1, canvas->displayRect[1], targetX2 + 1, canvas->displayRect[3], 0xFF000000);
	graphics->drawLine(canvas->displayRect[0], targetY2 + 1, canvas->displayRect[2], targetY2 + 1, 0xFF000000);
	graphics->drawLine(targetX2, canvas->displayRect[1], targetX2, canvas->displayRect[3], 0xFFBDFD80);
	graphics->drawLine(canvas->displayRect[0], targetY2, canvas->displayRect[2], targetY2, 0xFFBDFD80);
	return b3 && b4;
}


static void initTravelMap(Canvas* canvas) {


	canvas->TM_LoadLevelId = (short)std::max(1, std::min(canvas->loadMapID, 9));
	canvas->TM_LastLevelId = ((canvas->getRecentLoadType() == 1 && !canvas->TM_NewGame) ? canvas->TM_LoadLevelId : ((short)std::max(0, std::min(canvas->lastMapID, 9))));

	if (canvas->TM_LastLevelId == 0 && canvas->TM_LoadLevelId > 1) {
		canvas->TM_LastLevelId = (short)(canvas->TM_LoadLevelId - 1);
	}
	short n;
	if (canvas->TM_LoadLevelId > canvas->TM_LastLevelId) {
		n = 0;
	}
	else if (canvas->TM_LoadLevelId == canvas->TM_LastLevelId) {
		n = 1;
	}
	else {
		n = 2;
	}
	canvas->app->game->scriptStateVars[15] = n;

	// Load travel map config from per-level YAML
	loadTravelMapConfig(canvas);

	const auto& imgs = s_tmConfig.images;

	// Resolve sprite name → filename via SpriteDefs, then load image
	auto loadImg = [&](const std::string& name) -> Image* {
		const SpriteSource* src = SpriteDefs::getSource(name);
		const std::string& file = src ? src->file : name; // fallback to raw name
		return canvas->app->loadImage(const_cast<char*>(file.c_str()), true);
	};

	canvas->app->beginImageLoading();
	canvas->imgNameHighlight = loadImg(imgs.highlight);
	canvas->imgMagGlass = loadImg(imgs.magnifyingGlass);
	canvas->imgSpaceShip = loadImg(imgs.spaceship);

	const auto& loadPlanetName = getPlanetForLevel(canvas->TM_LoadLevelId);
	const auto& lastPlanetName = getPlanetForLevel(canvas->TM_LastLevelId);
	const auto* loadPlanet = getPlanetConfig(loadPlanetName);

	bool b = false;
	if (loadPlanet) {
		canvas->imgTierCloseUp = loadImg(loadPlanet->closeupImage);

		// Load arrival closeup if arriving from a different planet
		if (loadPlanet->hasArrivalCloseup && loadPlanetName != lastPlanetName) {
			canvas->imgEarthCloseUp = loadImg(loadPlanet->arrivalCloseupImage);
			b = true;
		}
	}

	// Load travel path image from matching path config
	const auto* path = findPath(lastPlanetName, loadPlanetName);
	if (path) {
		canvas->imgTravelPath = loadImg(path->image);
	}

	canvas->imgTravelBG = loadImg(imgs.background);
	canvas->imgMapHorzGridLines = loadImg(imgs.horzGrid);
	canvas->imgMapVertGridLines = loadImg(imgs.vertGrid);
	canvas->app->endImageLoading();

	canvas->totalTMTimeInPastAnimations = 0;
	canvas->mapWidth = canvas->imgTravelBG->width;
	canvas->mapHeight = canvas->imgTravelBG->height;
	canvas->xDiff = std::max(0, (canvas->displayRect[2] - canvas->imgTravelBG->width) / 2);
	canvas->yDiff = std::max(0, (canvas->displayRect[3] - canvas->imgTravelBG->height) / 2);

	int n2 = 0;
	int n3 = 0;
	if (loadPlanet) {
		n2 = loadPlanet->origin[0];
		n3 = loadPlanet->origin[1];
	}

	if (!b) {
		const auto& ld = s_tmConfig.loadLevel;
		canvas->targetX = ld.crosshair[0] + canvas->xDiff + n2;
		canvas->targetY = ld.crosshair[1] + canvas->yDiff + n3;
	}
	else {
		if (loadPlanet && loadPlanet->hasSpecialMarker) {
			canvas->targetX = loadPlanet->specialMarker[0] + canvas->xDiff + n2;
			canvas->targetY = loadPlanet->specialMarker[1] + canvas->yDiff + n3;
		}
	}

	canvas->stateVars[0] = canvas->app->upTimeMs;;
	if (b) {
		canvas->stateVars[8] = 1;
	}

	// Unused
	canvas->imgStarField = canvas->app->loadImage("cockpit.bmp", true);
	canvas->starFieldWidth = Applet::IOS_WIDTH;
	canvas->starFieldHeight = canvas->imgStarField->height;
	canvas->starFieldScrollY = 0;
	canvas->starFieldZoom = 1;
	canvas->starFieldBuffer.assign(
	    (size_t)canvas->starFieldWidth * (size_t)canvas->starFieldHeight, 0);
}


static void disposeTravelMap(Canvas* canvas) {
	canvas->imgNameHighlight->~Image();
	canvas->imgNameHighlight = nullptr;
	canvas->imgMagGlass->~Image();
	canvas->imgMagGlass = nullptr;
	canvas->imgTravelBG->~Image();
	canvas->imgTravelBG = nullptr;
	canvas->imgTravelPath->~Image();
	canvas->imgTravelPath = nullptr;
	canvas->imgSpaceShip->~Image();
	canvas->imgSpaceShip = nullptr;
	canvas->imgTierCloseUp->~Image();
	canvas->imgTierCloseUp = nullptr;
	canvas->imgEarthCloseUp->~Image();
	canvas->imgEarthCloseUp = nullptr;
	canvas->imgMapHorzGridLines->~Image();
	canvas->imgMapHorzGridLines = nullptr;
	canvas->imgMapVertGridLines->~Image();
	canvas->imgMapVertGridLines = nullptr;

	// Unused
	canvas->imgStarField->~Image();
	canvas->imgStarField = nullptr;
}


static void drawStarFieldPage(Canvas* canvas, Graphics* graphics) {


	if (canvas->app->upTimeMs - canvas->stateVars[0] > 5250) {
		canvas->starFieldZoom = 2u;
		finishTravelMapAndLoadLevel(canvas);
	}
	else {
		int yPos = canvas->screenRect[3] - canvas->screenRect[1] - canvas->imgStarField->height;
		graphics->fillRect(0, 0, canvas->menuRect[2], canvas->menuRect[3], 0xFF000000);
		drawStarField(canvas, graphics, 1, yPos / 2);
		graphics->drawImage(canvas->imgStarField, 1, yPos / 2, 0, 0, 0);
		graphics->drawImage(canvas->imgStarField, canvas->screenRect[2] - 1, yPos / 2, 24, 4, 0);
		canvas->app->canvas->softKeyRightID = -1;
		canvas->app->canvas->softKeyLeftID = -1;
		canvas->app->canvas->repaintFlags |= Canvas::REPAINT_SOFTKEYS;
		if (canvas->app->canvas->displaySoftKeys) {
			canvas->app->canvas->softKeyRightID = 40;
			canvas->app->canvas->repaintFlags |= Canvas::REPAINT_SOFTKEYS;
		}
	}
}


static void drawStarField(Canvas* canvas, Graphics* graphics, int x, int y) {

	int upTimeMs;
	int result;
	int sfHeight;
	unsigned int pixelValue;
	unsigned int starType;
	int distX;
	int distY;
	signed int combinedDist;
	int avgDist;
	int starSize;
	int scaledDist;
	int i;
	int drawX;
	int offsetX;
	int halfWidth;
	int halfHeight;
	int row;
	int drawY;
	int offsetY;

	graphics->fillRect(x, y, canvas->starFieldWidth, canvas->starFieldHeight, 0xFF000000);
	if (canvas->app->upTimeMs - canvas->stateVars[7] > 59) {
		canvas->stateVars[7] = canvas->app->upTimeMs;
		runStarFieldFrame(canvas);
	}

	halfWidth = canvas->starFieldWidth / 2;
	sfHeight = canvas->starFieldHeight;
	drawY = y;
	row = 0;
	halfHeight = sfHeight / 2;
	offsetY = sfHeight / -2;
next_star_row:
	if (row < sfHeight - canvas->starFieldScrollY)
	{
		drawX = x;
		offsetX = -halfWidth;
		for (i = 0; ; ++i)
		{
			if (i >= canvas->starFieldWidth)
			{
				++row;
				++drawY;
				++offsetY;
				sfHeight = canvas->starFieldHeight;
				goto next_star_row;
			}
			pixelValue = canvas->starFieldBuffer[i + canvas->starFieldWidth * row];
			starType = pixelValue >> 10;
			if ((pixelValue & 0x3FF) != 0)
				break;
		next_star_pixel:
			++drawX;
			++offsetX;
		}
		distX = (int)(abs(offsetX) << 10) / halfWidth;
		distY = (int)(abs(offsetY) << 10) / halfHeight;
		combinedDist = distX + distY + ((unsigned int)(distX + distY) >> 31);
		avgDist = (distX + distY) / 2;
		graphics->setColor(65793 * ((avgDist << 7 >> 10) + 128) - 0x1000000);
		if (starType == 1)
		{
			scaledDist = 8 * avgDist;
		}
		else
		{
			if (!starType || starType != 2)
			{
				starSize = combinedDist >> 11;
			draw_star:
				if (starSize <= 0 || starSize == 1)
				{
					graphics->fillRect(drawX, drawY, 1, 1);
				}
				else
				{
					graphics->fillCircle(drawX, drawY, starSize);
				}
				goto next_star_pixel;
			}
			scaledDist = 10 * avgDist;
		}
		starSize = scaledDist >> 10;
		goto draw_star;
	}
}


static void runStarFieldFrame(Canvas* canvas) {


	int fieldWidth;
	int fieldHeight;
	unsigned int pixelValue;
	unsigned int starType;
	int radiusY;
	signed int radiusX;
	int offsetY;
	int col;
	int colOffset;
	unsigned int rowDist;
	unsigned int btmPixelValue;
	unsigned int btmStarType;
	int btmRadiusY;
	signed int btmRadiusX;
	int btmOffsetY;
	int btmCol;
	int btmColOffset;
	int spawnIndex;
	unsigned int btmRowDist;
	int randAngle;
	int starAngle;
	int randType;
	int spawnX;
	int spawnY;
	int result;
	int btmZoom;
	int zoom;
	int btmSmallRadius;
	int btmLargeRadius;
	int smallRadius;
	int largeRadius;
	int halfWidth;
	int halfHeight;
	int btmAngle;
	int btmOffsetXResult;
	int i;
	int btmRowOffset;
	int offsetXResult;
	int angle;
	int rowOffset;
	int btmRow;

	fieldWidth = canvas->starFieldWidth;
	halfWidth = fieldWidth / 2;
	fieldHeight = canvas->starFieldHeight;
	halfHeight = fieldHeight / 2;
	for (i = 0; i < fieldHeight / 2; ++i)
	{
		col = 0;
		rowOffset = halfHeight - i;
		rowDist = abs(halfHeight - i);
		largeRadius = 60 * rowDist;
		smallRadius = 20 * rowDist;
		colOffset = -halfWidth;
		while (col < fieldWidth)
		{
			pixelValue = canvas->starFieldBuffer[col + fieldWidth * i];
			angle = pixelValue & 0x3FF;
			if ((pixelValue & 0x3FF) != 0)
			{
				starType = pixelValue >> 10;
				if (starType == 1)
				{
					radiusY = smallRadius;
					radiusX = 20 * abs(colOffset);
				}
				else if (starType)
				{
					radiusY = largeRadius;
					radiusX = 60 * abs(colOffset);
				}
				else
				{
					radiusY = 300;
					radiusX = 300;
				}
				zoom = canvas->starFieldZoom;
				{
					double rad = (double)angle * (2.0 * std::numbers::pi / 1024.0);
					offsetXResult = (int)(std::cos(rad) * (double)radiusX / (double)zoom / (double)halfWidth);
					offsetY = (int)(std::sin(rad) * (double)radiusY / (double)zoom / (double)halfHeight);
				}
				if (rowOffset + offsetY >= -halfHeight && halfHeight > rowOffset + offsetY && -halfWidth <= colOffset + offsetXResult && halfWidth > colOffset + offsetXResult)
				{
					canvas->starFieldBuffer[col + fieldWidth * (i - offsetY) + offsetXResult] = angle | ((short)starType << 10);
				}
				canvas->starFieldBuffer[col + fieldWidth * i] = 0;
			}
			++col;
			++colOffset;
		}
		fieldHeight = canvas->starFieldHeight;
	}
	btmRow = fieldHeight - 1;
	btmRowOffset = halfHeight - (fieldHeight - 1);
	while (btmRow >= fieldHeight / 2)
	{
		btmCol = 0;
		btmRowDist = abs(btmRowOffset);
		btmColOffset = -halfWidth;
		btmLargeRadius = 60 * btmRowDist;
		btmSmallRadius = 20 * btmRowDist;
		while (btmCol < fieldWidth)
		{
			btmPixelValue = canvas->starFieldBuffer[btmCol + fieldWidth * btmRow];
			btmAngle = btmPixelValue & 0x3FF;
			if ((btmPixelValue & 0x3FF) != 0)
			{
				btmStarType = btmPixelValue >> 10;
				if (btmStarType == 1)
				{
					btmRadiusY = btmSmallRadius;
					btmRadiusX = 20 * abs(btmColOffset);
				}
				else if (btmStarType)
				{
					btmRadiusY = btmLargeRadius;
					btmRadiusX = 60 * abs(btmColOffset);
				}
				else
				{
					btmRadiusY = 300;
					btmRadiusX = 300;
				}
				btmZoom = canvas->starFieldZoom;
				{
					double rad = (double)btmAngle * (2.0 * std::numbers::pi / 1024.0);
					btmOffsetXResult = (int)(std::cos(rad) * (double)btmRadiusX / (double)btmZoom / (double)halfWidth);
					btmOffsetY = (int)(std::sin(rad) * (double)btmRadiusY / (double)btmZoom / (double)halfHeight);
				}
				if (btmRowOffset + btmOffsetY >= -halfHeight && halfHeight > btmRowOffset + btmOffsetY && btmColOffset + btmOffsetXResult >= -halfWidth && halfWidth > btmColOffset + btmOffsetXResult)
				{
					canvas->starFieldBuffer[btmCol + fieldWidth * (btmRow - btmOffsetY) + btmOffsetXResult] = btmAngle | ((short)btmStarType << 10);
				}
				canvas->starFieldBuffer[btmCol + fieldWidth * btmRow] = 0;
			}
			++btmCol;
			++btmColOffset;
		}
		--btmRow;
		++btmRowOffset;
		fieldHeight = canvas->starFieldHeight;
	}
	spawnIndex = 0;
	while (1)
	{
		result = 4 / canvas->starFieldZoom;
		if (spawnIndex >= result)
			break;
		++spawnIndex;
		randAngle = canvas->app->nextInt() % 1023;
		starAngle = randAngle + 1;
		randType = canvas->app->nextInt() % 3;
		spawnX = canvas->app->nextInt() % 15 + 1;
		if ((unsigned int)(randAngle - 256) <= 0x1FE)
			spawnX = -spawnX;
		spawnY = canvas->app->nextInt() % 15 + 1;
		if (starAngle >= 512)
			spawnY = -spawnY;
		canvas->starFieldBuffer[spawnX + halfWidth + canvas->starFieldWidth * (halfHeight - spawnY)] = starAngle | ((short)randType << 10);
	}
}



void TravelMapState::onEnter(Canvas* canvas) {
	if (CAppContainer::getInstance()->skipTravelMap) {
		canvas->setLoadingBarText((short)0, (short)41);
		canvas->setState(Canvas::ST_LOADING);
		return;
	}
	initTravelMap(canvas);
}

void TravelMapState::onExit(Canvas* canvas) {
}

void TravelMapState::update(Canvas* canvas) {
}

void TravelMapState::render(Canvas* canvas, Graphics* graphics) {
	drawTravelMap(canvas, graphics);
}

bool TravelMapState::handleInput(Canvas* canvas, int key, int action) {
	handleTravelMapInput(canvas, key, action);
	return true;
}
