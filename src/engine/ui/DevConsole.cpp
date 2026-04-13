#include "DevConsole.h"
#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_opengl2.h"
#include <SDL.h>

#include "CAppContainer.h"
#include "App.h"
#include "Canvas.h"
#include "Game.h"
#include "Entity.h"
#include "EntityDef.h"
#include "EntityMonster.h"
#include "CombatEntity.h"
#include "AIComponent.h"
#include "Player.h"
#include "Combat.h"
#include "Render.h"
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
	ImGui_ImplOpenGL2_Init();

	initialized = true;
}

void DevConsole::shutdown() {
	if (!initialized) return;
	ImGui_ImplOpenGL2_Shutdown();
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

	if (!visible) return false;

	ImGui_ImplSDL2_ProcessEvent(&event);

	// If ImGui wants keyboard or mouse, consume the event
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
	if (!initialized || !visible) return;
	ImGui_ImplOpenGL2_NewFrame();
	ImGui_ImplSDL2_NewFrame();
	ImGui::NewFrame();
}

void DevConsole::render() {
	if (!initialized || !visible) return;

	auto* container = CAppContainer::getInstance();
	if (!container || !container->app) return;

	// Main menu bar
	if (ImGui::BeginMainMenuBar()) {
		if (ImGui::BeginMenu("View")) {
			ImGui::MenuItem("Game State", nullptr, &showDemo);
			ImGui::EndMenu();
		}
		ImGui::Separator();
		ImGui::Text("State: %d", container->app->canvas->state);
		ImGui::Separator();
		ImGui::Text("F12 to close");
		ImGui::EndMainMenuBar();
	}

	drawGameState();
	drawEntityInspector();
	drawScriptState();
	drawPerformance();

	if (showDemo) {
		ImGui::ShowDemoWindow(&showDemo);
	}

	ImGui::Render();
	ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());
}

void DevConsole::drawGameState() {
	auto* app = CAppContainer::getInstance()->app;
	if (!app) return;

	ImGui::SetNextWindowSize(ImVec2(320, 280), ImGuiCond_FirstUseEver);
	if (!ImGui::Begin("Game State")) {
		ImGui::End();
		return;
	}

	if (ImGui::CollapsingHeader("Player", ImGuiTreeNodeFlags_DefaultOpen)) {
		Player* player = app->player.get();
		if (player && player->ce) {
			CombatEntity* ce = player->ce;
			int hp = ce->getStat(0);     // current health
			int maxHp = ce->getStat(1);  // max health
			int armor = ce->getStat(2);
			int def = ce->getStat(3);
			int str = ce->getStat(4);
			int acc = ce->getStat(5);
			int agi = ce->getStat(6);

			ImGui::Text("HP: %d / %d", hp, maxHp);
			ImGui::Text("Armor: %d  Def: %d  Str: %d", armor, def, str);
			ImGui::Text("Acc: %d  Agi: %d", acc, agi);
			ImGui::Text("Level: %d  XP: %d / %d", player->level, player->currentXP, player->nextLevelXP);
			ImGui::Text("Weapons: 0x%X  Gold: %d", player->weapons, player->goldCopy);
			ImGui::Text("Moves: %d  Deaths: %d", player->totalMoves, player->totalDeaths);
			ImGui::Text("Noclip: %s  God: %s",
				player->noclip ? "ON" : "off",
				player->god ? "ON" : "off");
		}
	}

	if (ImGui::CollapsingHeader("Map")) {
		Game* game = app->game.get();
		if (game) {
			ImGui::Text("Entities: %d", game->numEntities);
			ImGui::Text("Monsters: %d (active list)", game->numMonsters);
			ImGui::Text("Secrets: %d / %d", game->mapSecretsFound, game->totalSecrets);
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
	ImGui::SliderInt("Entity #", &selectedEntity, 0, game->numEntities - 1);

	Entity& ent = game->entities[selectedEntity];
	EntityDef* def = ent.def;

	if (def) {
		ImGui::Text("Tile: %d  Type: %d  SubType: %d  Parm: %d",
			def->tileIndex, def->eType, def->eSubType, def->parm);
		ImGui::Text("RenderFlags: 0x%X", def->renderFlags);
		ImGui::Text("MonsterIdx: %d", def->monsterIdx);
	} else {
		ImGui::TextDisabled("(no EntityDef)");
	}

	ImGui::Separator();
	ImGui::Text("Pos: [%d, %d]", ent.pos[0], ent.pos[1]);
	ImGui::Text("Info: 0x%X  Param: %d", ent.info, ent.param);
	ImGui::Text("MonsterFlags: 0x%X  Effects: 0x%X", ent.monsterFlags, ent.monsterEffects);

	if (ent.combat) {
		ImGui::Separator();
		ImGui::Text("Combat - HP: %d/%d  Weapon: %d",
			ent.combat->getStat(0), ent.combat->getStat(1), ent.combat->weapon);
	}
	if (ent.ai) {
		ImGui::Text("AI - GoalType: %d  GoalTurns: %d",
			ent.ai->goalType, ent.ai->goalTurns);
		ImGui::Text("AI - Goal: [%d, %d]", ent.ai->goalX, ent.ai->goalY);
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
		// Show non-zero state vars (compact view)
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

void DevConsole::drawPerformance() {
	ImGui::SetNextWindowSize(ImVec2(250, 100), ImGuiCond_FirstUseEver);
	if (!ImGui::Begin("Performance")) {
		ImGui::End();
		return;
	}

	ImGuiIO& io = ImGui::GetIO();
	ImGui::Text("FPS: %.1f (%.3f ms/frame)", io.Framerate, 1000.0f / io.Framerate);

	ImGui::End();
}
