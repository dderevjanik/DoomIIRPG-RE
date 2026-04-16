#include "TravelMapState.h"
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
#include "TinyGL.h"
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

	int upTimeMs; // r2
	int result; // r0
	int sfHeight; // r2
	unsigned int v8; // r2
	unsigned int v9; // r11
	int v10; // r6
	int v11; // r0
	signed int v12; // r5
	int v13; // r6
	int v14; // r3
	int v15; // r0
	int i; // r4
	int v17; // r10
	int v18; // r8
	int v23; // [sp+18h] [bp-2Ch]
	int v24; // [sp+1Ch] [bp-28h]
	int v25; // [sp+20h] [bp-24h]
	int v26; // [sp+24h] [bp-20h]
	int v27; // [sp+28h] [bp-1Ch]

	graphics->fillRect(x, y, canvas->starFieldWidth, canvas->starFieldHeight, 0xFF000000);
	if (canvas->app->upTimeMs - canvas->stateVars[7] > 59) {
		canvas->stateVars[7] = canvas->app->upTimeMs;
		runStarFieldFrame(canvas);
	}

	v23 = canvas->starFieldWidth / 2;
	sfHeight = canvas->starFieldHeight;
	v26 = y;
	v25 = 0;
	v24 = sfHeight / 2;
	v27 = sfHeight / -2;
LABEL_20:
	if (v25 < sfHeight - canvas->starFieldScrollY)
	{
		v17 = x;
		v18 = -v23;
		for (i = 0; ; ++i)
		{
			if (i >= canvas->starFieldWidth)
			{
				++v25;
				++v26;
				++v27;
				sfHeight = canvas->starFieldHeight;
				goto LABEL_20;
			}
			v8 = canvas->app->tinyGL->pixels[i + canvas->starFieldWidth * v25];
			v9 = v8 >> 10;
			if ((v8 & 0x3FF) != 0)
				break;
		LABEL_17:
			++v17;
			++v18;
		}
		v10 = (int)(abs(v18) << 10) / v23;
		v11 = (int)(abs(v27) << 10) / v24;
		v12 = v10 + v11 + ((unsigned int)(v10 + v11) >> 31);
		v13 = (v10 + v11) / 2;
		graphics->setColor(65793 * ((v13 << 7 >> 10) + 128) - 0x1000000);
		if (v9 == 1)
		{
			v15 = 8 * v13;
		}
		else
		{
			if (!v9 || v9 != 2)
			{
				v14 = v12 >> 11;
			LABEL_9:
				if (v14 <= 0 || v14 == 1)
				{
					graphics->fillRect(v17, v26, 1, 1);
				}
				else
				{
					graphics->fillCircle(v17, v26, v14);
				}
				goto LABEL_17;
			}
			v15 = 10 * v13;
		}
		v14 = v15 >> 10;
		goto LABEL_9;
	}
}


static void runStarFieldFrame(Canvas* canvas) {


	int v1; // r11
	int v2; // r3
	TinyGL* tinyGL; // r5
	unsigned int v4; // r3
	unsigned int v5; // r3
	int v6; // r6
	signed int v7; // r0
	int* v8; // r10
	int v9; // r0
	int v10; // r4
	int v11; // r8
	unsigned int v12; // r3
	TinyGL* v13; // r5
	unsigned int v14; // r3
	unsigned int v15; // r3
	int v16; // r6
	signed int v17; // r0
	int* v18; // r10
	int v19; // r0
	int v20; // r4
	int v21; // r8
	int v22; // r11
	unsigned int v23; // r3
	int v24; // r6
	int v25; // r10
	int v26; // r4
	int v27; // r5
	int v28; // r0
	int result; // r0
	int v30; // [sp+0h] [bp-78h]
	int v31; // [sp+4h] [bp-74h]
	int v32; // [sp+8h] [bp-70h]
	int v33; // [sp+Ch] [bp-6Ch]
	int v34; // [sp+10h] [bp-68h]
	int v35; // [sp+14h] [bp-64h]
	int v38; // [sp+20h] [bp-58h]
	int v39; // [sp+24h] [bp-54h]
	int v40; // [sp+28h] [bp-50h]
	int v41; // [sp+30h] [bp-48h]
	int i; // [sp+34h] [bp-44h]
	int v43; // [sp+3Ch] [bp-3Ch]
	unsigned short* v44; // [sp+44h] [bp-34h]
	unsigned short* pixels; // [sp+48h] [bp-30h]
	int v46; // [sp+4Ch] [bp-2Ch]
	int v47; // [sp+54h] [bp-24h]
	int v48; // [sp+58h] [bp-20h]
	int v49; // [sp+5Ch] [bp-1Ch]

	v1 = canvas->starFieldWidth;
	v38 = v1 / 2;
	v2 = canvas->starFieldHeight;
	v39 = v2 / 2;
	for (i = 0; i < v2 / 2; ++i)
	{
		v10 = 0;
		v48 = v39 - i;
		v12 = abs(v39 - i);
		v35 = 60 * v12;
		v34 = 20 * v12;
		v11 = -v38;
		while (v10 < v1)
		{
			tinyGL = canvas->app->tinyGL.get();
			pixels = tinyGL->pixels;
			v4 = pixels[v10 + v1 * i];
			v47 = v4 & 0x3FF;
			if ((v4 & 0x3FF) != 0)
			{
				v5 = v4 >> 10;
				if (v5 == 1)
				{
					v6 = v34;
					v7 = 20 * abs(v11);
				}
				else if (v5)
				{
					v6 = v35;
					v7 = 60 * abs(v11);
				}
				else
				{
					v6 = 300;
					v7 = 300;
				}
				v31 = canvas->starFieldZoom;
				v8 = canvas->app->render->sinTable;
				v46 = (v8[(v47 + 256) & 0x3FF] * (v7 / v31) / v38) >> 16;
				v9 = (v8[v47] * (v6 / v31) / v39) >> 16;
				if (v48 + v9 >= -v39 && v39 > v48 + v9 && -v38 <= v11 + v46 && v38 > v11 + v46)
				{
					pixels[v10 + v1 * (i - v9) + v46] = v47 | ((short)v5 << 10);
					v1 = canvas->starFieldWidth;
					tinyGL = canvas->app->tinyGL.get();
				}
				tinyGL->pixels[v10 + v1 * i] = 0;
				v1 = canvas->starFieldWidth;
			}
			++v10;
			++v11;
		}
		v2 = canvas->starFieldHeight;
	}
	v49 = v2 - 1;
	v43 = v39 - (v2 - 1);
	while (v49 >= v2 / 2)
	{
		v20 = 0;
		v23 = abs(v43);
		v21 = -v38;
		v33 = 60 * v23;
		v32 = 20 * v23;
		while (v20 < v1)
		{
			v13 = canvas->app->tinyGL.get();
			v44 = v13->pixels;
			v14 = v44[v20 + v1 * v49];
			v40 = v14 & 0x3FF;
			if ((v14 & 0x3FF) != 0)
			{
				v15 = v14 >> 10;
				if (v15 == 1)
				{
					v16 = v32;
					v17 = 20 * abs(v21);
				}
				else if (v15)
				{
					v16 = v33;
					v17 = 60 * abs(v21);
				}
				else
				{
					v16 = 300;
					v17 = 300;
				}
				v30 = canvas->starFieldZoom;
				v18 = canvas->app->render->sinTable;
				v41 = (v18[(v40 + 256) & 0x3FF] * (v17 / v30) / v38) >> 16;
				v19 = (v18[v40] * (v16 / v30) / v39) >> 16;
				if (v43 + v19 >= -v39 && v39 > v43 + v19 && v21 + v41 >= -v38 && v38 > v21 + v41)
				{
					v44[v20 + v1 * (v49 - v19) + v41] = v40 | ((short)v15 << 10);
					v1 = canvas->starFieldWidth;
					v13 = canvas->app->tinyGL.get();
				}
				v13->pixels[v20 + v1 * v49] = 0;
				v1 = canvas->starFieldWidth;
			}
			++v20;
			++v21;
		}
		--v49;
		++v43;
		v2 = canvas->starFieldHeight;
	}
	v22 = 0;
	while (1)
	{
		result = 4 / canvas->starFieldZoom;
		if (v22 >= result)
			break;
		++v22;
		v24 = canvas->app->nextInt() % 1023;
		v25 = v24 + 1;
		v26 = canvas->app->nextInt() % 3;
		v27 = canvas->app->nextInt() % 15 + 1;
		if ((unsigned int)(v24 - 256) <= 0x1FE)
			v27 = -v27;
		v28 = canvas->app->nextInt() % 15 + 1;
		if (v25 >= 512)
			v28 = -v28;
		canvas->app->tinyGL->pixels[v27 + v38 + canvas->starFieldWidth * (v39 - v28)] = v25 | ((short)v26 << 10);
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
