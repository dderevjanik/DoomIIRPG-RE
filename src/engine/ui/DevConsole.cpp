#include "DevConsole.h"
#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_opengl3.h"
#include <SDL.h>

#include "CAppContainer.h"
#include "ModManager.h"
#include "App.h"
#include "Canvas.h"
#include "Game.h"
#include "Entity.h"
#include "EntityDef.h"
#include "EntityMonster.h"
#include "CombatEntity.h"
#include "AIComponent.h"
#include "LootComponent.h"
#include "Player.h"
#include "Combat.h"
#include "Render.h"
#include "TinyGL.h"
#include "Sound.h"
#include "Hud.h"

DevConsole::DevConsole() = default;

DevConsole::~DevConsole() {
	if (initialized) {
		shutdown();
	}
}

void DevConsole::init(SDL_Window* window, void* glContext) {
	if (initialized) return;

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	io.IniFilename = "imgui.ini";

	ImGui::StyleColorsDark();
	ImGuiStyle& style = ImGui::GetStyle();
	style.Alpha = 0.92f;
	style.WindowRounding = 4.0f;

	ImGui_ImplSDL2_InitForOpenGL(window, glContext);
	// Match the rest of the engine's shader compatibility (B2 migration uses
	// GLSL 1.10 / GLES 2.0 dialect). The OpenGL3 backend autodetects on
	// nullptr, but pinning to 110 keeps us off the in/out path and works on
	// macOS's GL 2.1 compat profile alongside our textureShader/worldShader.
	ImGui_ImplOpenGL3_Init("#version 110");

	initialized = true;
}

void DevConsole::shutdown() {
	if (!initialized) return;
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplSDL2_Shutdown();
	ImGui::DestroyContext();
	initialized = false;
}

bool DevConsole::processEvent(const SDL_Event& event) {
	if (!initialized) return false;

	// Toggle on F12 or backtick
	if (event.type == SDL_KEYDOWN && !event.key.repeat) {
		if (event.key.keysym.scancode == SDL_SCANCODE_F12 ||
		    event.key.keysym.scancode == SDL_SCANCODE_GRAVE) {
			toggle();
			return true;
		}
	}

	// Always forward to ImGui when initialized, so any ImGui-using subsystem
	// (e.g. the map editor state) can receive mouse/keyboard events even when
	// the dev-console itself is hidden. ImGui's WantCapture* decides whether
	// to swallow the event from the game input system.
	ImGui_ImplSDL2_ProcessEvent(&event);

	ImGuiIO& io = ImGui::GetIO();
	if (io.WantCaptureKeyboard && (event.type == SDL_KEYDOWN || event.type == SDL_KEYUP || event.type == SDL_TEXTINPUT)) {
		return true;
	}
	if (io.WantCaptureMouse && (event.type == SDL_MOUSEBUTTONDOWN || event.type == SDL_MOUSEBUTTONUP ||
	                            event.type == SDL_MOUSEMOTION || event.type == SDL_MOUSEWHEEL)) {
		return true;
	}
	return false;
}

void DevConsole::newFrame() {
	// Always start a new ImGui frame when initialized so other subsystems
	// (e.g. the map editor state) can submit ImGui windows even while the
	// dev-console itself is hidden. The `visible` flag only gates the
	// dev-console's own panels inside render().
	if (!initialized) return;
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplSDL2_NewFrame();
	ImGui::NewFrame();
}

void DevConsole::render() {
	if (!initialized) return;

	auto* container = CAppContainer::getInstance();

	if (visible && container && container->app) {
		int canvasState = container->app->canvas ? container->app->canvas->state : -1;

		// Main menu bar
		if (ImGui::BeginMainMenuBar()) {
			if (ImGui::BeginMenu("View")) {
				ImGui::MenuItem("ImGui Demo", nullptr, &showDemo);
				ImGui::EndMenu();
			}
			ImGui::Separator();
			ImGui::Text("%s", stateName(canvasState));
			ImGui::Separator();
			ImGuiIO& io = ImGui::GetIO();
			ImGui::Text("%.0f FPS", io.Framerate);
			ImGui::Separator();
			ImGui::TextDisabled("F12 to close");
			ImGui::EndMainMenuBar();
		}

		drawPerformance();
		drawRenderStats();
		drawGameState();
		drawEntityInspector();
		drawScriptState();
		drawMods();

		if (showDemo) {
			ImGui::ShowDemoWindow(&showDemo);
		}
	}

	// Always flush whatever windows were submitted this frame (may be none).
	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void DevConsole::drawPerformance() {
	auto* app = CAppContainer::getInstance()->app;
	if (!app) return;

	ImGuiIO& io = ImGui::GetIO();

	// Update frame time history
	float frameMs = 1000.0f / (io.Framerate > 0 ? io.Framerate : 1.0f);
	frameTimeHistory[frameTimeOffset] = frameMs;
	frameTimeOffset = (frameTimeOffset + 1) % FRAME_HISTORY_SIZE;

	ImGui::SetNextWindowSize(ImVec2(340, 220), ImGuiCond_FirstUseEver);
	if (!ImGui::Begin("Performance")) {
		ImGui::End();
		return;
	}

	// FPS and frame time
	ImGui::Text("FPS: %.1f  (%.2f ms/frame)", io.Framerate, frameMs);

	// Frame time graph
	float maxTime = 0;
	for (int i = 0; i < FRAME_HISTORY_SIZE; i++)
		if (frameTimeHistory[i] > maxTime) maxTime = frameTimeHistory[i];
	if (maxTime < 16.7f) maxTime = 16.7f; // minimum scale = 60fps line

	char overlay[32];
	snprintf(overlay, sizeof(overlay), "max %.1f ms", maxTime);
	ImGui::PlotLines("##frametime", frameTimeHistory, FRAME_HISTORY_SIZE, frameTimeOffset,
	                  overlay, 0.0f, maxTime * 1.2f, ImVec2(0, 60));

	// Render subsystem timing
	Render* render = app->render.get();
	if (render && ImGui::CollapsingHeader("Frame Breakdown")) {
		ImGui::Text("Frame:   %3d ms", render->frameTime);
		ImGui::Text("BSP:     %3d ms", render->bspTime);
	}

	// Memory
	if (ImGui::CollapsingHeader("Memory")) {
		ImGui::Text("Startup:  %d KB", app->startupMemory / 1024);
		ImGui::Text("Images:   %d KB", app->imageMemory / 1024);
		ImGui::Text("Peak:     %d KB", app->peakMemoryUsage / 1024);
		if (render) {
			ImGui::Text("Map:      %d KB", render->mapMemoryUsage / 1024);
			ImGui::Text("Texels:   %d KB", render->texelMemoryUsage / 1024);
			ImGui::Text("Palettes: %d KB", render->paletteMemoryUsage / 1024);
		}
	}

	ImGui::End();
}

void DevConsole::drawRenderStats() {
	auto* app = CAppContainer::getInstance()->app;
	if (!app) return;

	Render* render = app->render.get();
	TinyGL* tinyGL = app->tinyGL.get();
	if (!render) return;

	ImGui::SetNextWindowSize(ImVec2(280, 240), ImGuiCond_FirstUseEver);
	if (!ImGui::Begin("Render Stats")) {
		ImGui::End();
		return;
	}

	if (ImGui::CollapsingHeader("Geometry", ImGuiTreeNodeFlags_DefaultOpen)) {
		ImGui::Text("BSP Nodes:  %d", render->nodeCount);
		ImGui::Text("Sprites:    %d rendered / %d total", render->spriteRasterCount, render->spriteCount);
		ImGui::Text("Lines:      %d rendered / %d total", render->lineRasterCount, render->lineCount);
		if (tinyGL) {
			ImGui::Text("Quads:      %d total (%d clipped)",
				tinyGL->c_totalQuad, tinyGL->c_clippedQuad);
			ImGui::Text("Drawn:      %d polys, %d span px", tinyGL->countDrawn, tinyGL->spanPixels);
		}
	}

	if (ImGui::CollapsingHeader("Sprites")) {
		ImGui::Text("Map sprites:  %d", render->numMapSprites);
		ImGui::Text("Visible:      %d", render->viewSprites);
		ImGui::Text("Normal:       %d", render->numNormalSprites);
		ImGui::Text("Z-sorted:     %d", render->numZSprites);
	}

	if (ImGui::CollapsingHeader("Audio")) {
		Sound* sound = app->sound.get();
		if (sound) {
			int active = 0;
			for (size_t i = 0; i < sound->channel.size(); ++i) {
				if (sound->channel[i].sourceId != 0) active++;
			}
			ImGui::Text("Channels: %d active / %zu allocated (cap %zu)",
				active, sound->channel.size(), Sound::MAX_CHANNELS);
			ImGui::Text("Sounds: %s  Music: %s",
				sound->allowSounds ? "ON" : "off",
				sound->allowMusics ? "ON" : "off");
		}
	}

	ImGui::End();
}

void DevConsole::drawGameState() {
	auto* app = CAppContainer::getInstance()->app;
	if (!app) return;

	ImGui::SetNextWindowSize(ImVec2(320, 300), ImGuiCond_FirstUseEver);
	if (!ImGui::Begin("Game State")) {
		ImGui::End();
		return;
	}

	if (ImGui::CollapsingHeader("Player", ImGuiTreeNodeFlags_DefaultOpen)) {
		Player* player = app->player.get();
		if (player && player->ce) {
			CombatEntity* ce = player->ce;
			ImGui::Text("HP: %d / %d", ce->getStat(0), ce->getStat(1));
			ImGui::Text("Armor: %d  Def: %d  Str: %d", ce->getStat(2), ce->getStat(3), ce->getStat(4));
			ImGui::Text("Acc: %d  Agi: %d", ce->getStat(5), ce->getStat(6));
			ImGui::Text("Level: %d  XP: %d / %d", player->level, player->currentXP, player->nextLevelXP);
			ImGui::Text("Weapons: 0x%X  Gold: %d", player->weapons, player->goldCopy);
			ImGui::Text("Moves: %d  Deaths: %d", player->totalMoves, player->totalDeaths);
			ImGui::Text("Noclip: %s  God: %s",
				player->noclip ? "ON" : "off",
				player->god ? "ON" : "off");
		}
	}

	if (ImGui::CollapsingHeader("World", ImGuiTreeNodeFlags_DefaultOpen)) {
		Game* game = app->game.get();
		if (game) {
			ImGui::Text("Entities:  %d / %d", game->numEntities, CAppContainer::getInstance()->gameConfig.maxEntities);
			ImGui::Text("Monsters:  %d  AI: %d  Loot: %d", game->numMonsters, game->numAIComponents, game->numLootComponents);
			ImGui::Text("Secrets:   %d / %d", game->mapSecretsFound, game->totalSecrets);
			ImGui::Text("Sprites:   %d active, %d lerp", game->activeSprites, game->numLerpSprites);
			ImGui::Text("Scripts:   %d threads", game->numScriptThreads);
			ImGui::Text("Effects:   %d propagators, %d animating", game->activePropogators, game->animatingEffects);
		}
	}

	ImGui::End();
}

void DevConsole::drawEntityInspector() {
	auto* app = CAppContainer::getInstance()->app;
	if (!app) return;

	Game* game = app->game.get();
	if (!game || !game->entities) return;

	ImGui::SetNextWindowSize(ImVec2(380, 350), ImGuiCond_FirstUseEver);
	if (!ImGui::Begin("Entity Inspector")) {
		ImGui::End();
		return;
	}

	static int selectedEntity = 0;
	if (selectedEntity >= game->numEntities) selectedEntity = game->numEntities - 1;
	if (selectedEntity < 0) selectedEntity = 0;
	ImGui::SliderInt("Entity #", &selectedEntity, 0, game->numEntities - 1);

	Entity& ent = game->entities[selectedEntity];
	EntityDef* def = ent.def;

	if (def) {
		ImGui::Text("Tile: %d  Type: %d  SubType: %d  Parm: %d",
			def->tileIndex, def->eType, def->eSubType, def->parm);
		ImGui::Text("RenderFlags: 0x%X  MonsterIdx: %d", def->renderFlags, def->monsterIdx);
	} else {
		ImGui::TextDisabled("(no EntityDef)");
	}

	ImGui::Separator();
	ImGui::Text("Pos: [%d, %d]  Link: %d", ent.pos[0], ent.pos[1], ent.linkIndex);
	ImGui::Text("Info: 0x%X  Param: %d", ent.info, ent.param);
	ImGui::Text("Flags: 0x%X  Effects: 0x%X", ent.monsterFlags, ent.monsterEffects);

	if (ent.combat) {
		ImGui::Separator();
		ImGui::Text("Combat - HP: %d/%d  Weapon: %d",
			ent.combat->getStat(0), ent.combat->getStat(1), ent.combat->weapon);
		ImGui::Text("Combat - Def: %d  Str: %d  Acc: %d  Agi: %d",
			ent.combat->getStat(3), ent.combat->getStat(4), ent.combat->getStat(5), ent.combat->getStat(6));
	}
	if (ent.ai) {
		ImGui::Text("AI - Goal: type=%d turns=%d  pos=[%d,%d]",
			ent.ai->goalType, ent.ai->goalTurns, ent.ai->goalX, ent.ai->goalY);
	}
	if (ent.loot) {
		char buf[160];
		int off = snprintf(buf, sizeof(buf), "Loot: [");
		for (int i = 0; i < LootComponent::MAX_SLOTS && off < (int)sizeof(buf); ++i) {
			off += snprintf(buf + off, sizeof(buf) - off, "%s0x%X",
				i == 0 ? "" : ", ", ent.loot->lootSet[i]);
		}
		snprintf(buf + off, sizeof(buf) - off, "]");
		ImGui::Text("%s", buf);
	}

	ImGui::End();
}

void DevConsole::drawScriptState() {
	auto* app = CAppContainer::getInstance()->app;
	if (!app) return;

	Game* game = app->game.get();
	if (!game) return;

	ImGui::SetNextWindowSize(ImVec2(300, 250), ImGuiCond_FirstUseEver);
	if (!ImGui::Begin("Script State")) {
		ImGui::End();
		return;
	}

	if (ImGui::CollapsingHeader("State Variables", ImGuiTreeNodeFlags_DefaultOpen)) {
		bool anyNonZero = false;
		for (int i = 0; i < 128; i++) {
			if (game->scriptStateVars[i] != 0) {
				ImGui::Text("[%3d] = %d", i, game->scriptStateVars[i]);
				anyNonZero = true;
			}
		}
		if (!anyNonZero) {
			ImGui::TextDisabled("(all zero)");
		}
	}

	if (ImGui::CollapsingHeader("Canvas State Vars")) {
		Canvas* canvas = app->canvas.get();
		if (canvas) {
			for (int i = 0; i < Canvas::MAX_STATE_VARS; i++) {
				if (canvas->stateVars[i] != 0) {
					ImGui::Text("[%3d] = %d", i, canvas->stateVars[i]);
				}
			}
		}
	}

	ImGui::End();
}

const char* DevConsole::stateName(int state) {
	switch (state) {
		case 1:  return "Menu";
		case 2:  return "Intro Movie";
		case 3:  return "Playing";
		case 4:  return "Inter Camera";
		case 5:  return "Combat";
		case 6:  return "Automap";
		case 7:  return "Loading";
		case 8:  return "Dialog";
		case 9:  return "Intro";
		case 10: return "Benchmark";
		case 12: return "Minigame";
		case 13: return "Dying";
		case 14: return "Epilogue";
		case 15: return "Credits";
		case 16: return "Saving";
		case 17: return "Error";
		case 18: return "Camera";
		case 21: return "Travel Map";
		case 22: return "Char Select";
		case 23: return "Looting";
		case 24: return "Treadmill";
		case 25: return "Bot Dying";
		case 26: return "Logo";
		default: return "Unknown";
	}
}

void DevConsole::drawMods() {
	auto* container = CAppContainer::getInstance();
	if (!container || !container->modManager) return;

	const auto& mods = container->modManager->getLoadedMods();
	const auto& warnings = container->modManager->getWarnings();

	ImGui::SetNextWindowSize(ImVec2(400, 300), ImGuiCond_FirstUseEver);
	if (!ImGui::Begin("Mods")) {
		ImGui::End();
		return;
	}

	if (mods.empty()) {
		ImGui::TextDisabled("No mods loaded");
		ImGui::TextWrapped("Place mod directories or .zip files in the mods/ folder, or use --mod <dir> on the command line.");
	} else {
		ImGui::Text("%d mod(s) loaded", (int)mods.size());
		ImGui::Separator();

		for (const auto& mod : mods) {
			ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_DefaultOpen;
			bool open = ImGui::TreeNodeEx(mod.id.c_str(), flags, "%s%s%s",
				mod.name.c_str(),
				mod.version.empty() ? "" : (" v" + mod.version).c_str(),
				mod.isZip ? " [zip]" : "");

			if (open) {
				if (!mod.author.empty())
					ImGui::Text("Author: %s", mod.author.c_str());
				ImGui::Text("ID: %s", mod.id.c_str());
				ImGui::Text("Priority: %d", mod.priority);
				ImGui::Text("Path: %s", mod.path.c_str());

				if (!mod.depends.empty()) {
					ImGui::Text("Depends:");
					ImGui::SameLine();
					for (size_t i = 0; i < mod.depends.size(); i++) {
						if (i > 0) ImGui::SameLine();
						ImGui::TextColored(ImVec4(0.6f, 0.8f, 1.0f, 1.0f), "%s", mod.depends[i].c_str());
					}
				}

				if (!mod.conflicts.empty()) {
					ImGui::Text("Conflicts:");
					ImGui::SameLine();
					for (size_t i = 0; i < mod.conflicts.size(); i++) {
						if (i > 0) ImGui::SameLine();
						ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "%s", mod.conflicts[i].c_str());
					}
				}

				ImGui::TreePop();
			}
		}
	}

	if (!warnings.empty()) {
		ImGui::Separator();
		ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "Warnings (%d):", (int)warnings.size());
		for (const auto& w : warnings) {
			ImGui::TextWrapped("%s", w.c_str());
		}
	}

	ImGui::End();
}
