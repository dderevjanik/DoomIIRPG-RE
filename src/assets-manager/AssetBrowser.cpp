#include "AssetBrowser.h"

#include "imgui.h"
#include <SDL.h>

#include <algorithm>
#include <cstdio>
#include <cstring>

// Helper: case-insensitive substring match
static bool matchesFilter(const std::string& text, const char* filter) {
	if (!filter || filter[0] == '\0') return true;
	std::string lower = text;
	std::string filt = filter;
	std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
	std::transform(filt.begin(), filt.end(), filt.begin(), ::tolower);
	return lower.find(filt) != std::string::npos;
}

// Helper: read optional string from YAML node
static std::string yamlStr(const YAML::Node& node, const std::string& key, const std::string& def = "") {
	if (node[key]) return node[key].as<std::string>();
	return def;
}

static int yamlInt(const YAML::Node& node, const std::string& key, int def = 0) {
	if (node[key]) return node[key].as<int>();
	return def;
}

// ─── Data Loading ───────────────────────────────────────────────────────────

void AssetBrowser::init(const std::string& gameDir) {
	gameDir_ = gameDir;
	sprites_.load(gameDir);
	loadEntities();
	loadMonsters();
	loadWeapons();
	loadProjectiles();
	loadCharacters();
	loadEffects();
	loadAnimations();
	loaded_ = true;
}

void AssetBrowser::loadEntities() {
	try {
		YAML::Node root = YAML::LoadFile(gameDir_ + "/entities.yaml");
		YAML::Node ents = root["entities"];
		if (!ents) return;

		for (auto it = ents.begin(); it != ents.end(); ++it) {
			EntityEntry e;
			e.name = it->first.as<std::string>();
			auto& v = it->second;
			e.tileIndex = yamlInt(v, "tile_index");
			e.type = yamlStr(v, "type");
			e.subtype = yamlStr(v, "subtype");
			e.parm = yamlInt(v, "parm");
			if (v["text"]) {
				e.nameStr = yamlInt(v["text"], "name");
				e.longNameStr = yamlInt(v["text"], "long_name");
				e.descStr = yamlInt(v["text"], "description");
			}
			// Parse rendering/body_parts for composite preview
			if (v["rendering"]) {
				auto rend = v["rendering"];
				if (rend["flags"]) {
					for (const auto& flag : rend["flags"]) {
						auto f = flag.as<std::string>();
						if (f == "is_floater") e.isFloater = true;
						if (f == "is_special_boss") e.isSpecialBoss = true;
					}
				}
				if (rend["body_parts"]) {
					auto bp = rend["body_parts"];
					e.bodyParts.torsoZ = yamlInt(bp, "idle_torso_z");
					e.bodyParts.headZ = yamlInt(bp, "idle_head_z");
					e.bodyParts.headX = yamlInt(bp, "attack_head_x");
					e.noHeadOnAttack = bp["no_head_on_attack"] && bp["no_head_on_attack"].as<bool>();
				}
				if (rend["floater"]) e.isFloater = true;
				if (rend["special_boss"]) e.isSpecialBoss = true;
			}
			entities_.push_back(std::move(e));
		}
		std::fprintf(stderr, "Loaded %zu entities\n", entities_.size());
	} catch (const YAML::Exception& ex) {
		std::fprintf(stderr, "Failed to load entities.yaml: %s\n", ex.what());
	}
}

void AssetBrowser::loadMonsters() {
	try {
		YAML::Node root = YAML::LoadFile(gameDir_ + "/monsters.yaml");
		YAML::Node mons = root["monsters"];
		if (!mons) return;

		for (auto it = mons.begin(); it != mons.end(); ++it) {
			MonsterEntry m;
			m.name = it->first.as<std::string>();
			auto& v = it->second;
			m.index = yamlInt(v, "index");
			m.bloodColor = yamlStr(v, "blood_color");
			if (v["sounds"]) {
				auto s = v["sounds"];
				m.alertSnd1 = yamlStr(s, "alert1");
				m.alertSnd2 = yamlStr(s, "alert2");
				m.alertSnd3 = yamlStr(s, "alert3");
				m.attackSnd1 = yamlStr(s, "attack1");
				m.attackSnd2 = yamlStr(s, "attack2");
				m.idleSnd = yamlStr(s, "idle");
				m.painSnd = yamlStr(s, "pain");
				m.deathSnd = yamlStr(s, "death");
			}
			if (v["tiers"]) {
				for (const auto& tier : v["tiers"]) {
					MonsterTier t{};
					if (tier["stats"]) {
						auto st = tier["stats"];
						t.health = yamlInt(st, "health");
						t.armor = yamlInt(st, "armor");
						t.defense = yamlInt(st, "defense");
						t.strength = yamlInt(st, "strength");
						t.accuracy = yamlInt(st, "accuracy");
						t.agility = yamlInt(st, "agility");
					}
					if (tier["attacks"]) {
						auto a = tier["attacks"];
						t.attack1 = yamlStr(a, "attack1");
						t.attack2 = yamlStr(a, "attack2");
						t.chance = yamlInt(a, "chance");
					}
					if (tier["weakness"]) {
						for (auto wit = tier["weakness"].begin(); wit != tier["weakness"].end(); ++wit) {
							t.weaknesses.emplace_back(
								wit->first.as<std::string>(),
								wit->second.as<float>()
							);
						}
					}
					m.tiers.push_back(std::move(t));
				}
			}
			monsters_.push_back(std::move(m));
		}
		std::fprintf(stderr, "Loaded %zu monsters\n", monsters_.size());
	} catch (const YAML::Exception& ex) {
		std::fprintf(stderr, "Failed to load monsters.yaml: %s\n", ex.what());
	}
}

void AssetBrowser::loadWeapons() {
	try {
		YAML::Node root = YAML::LoadFile(gameDir_ + "/weapons.yaml");
		YAML::Node weps = root["weapons"];
		if (!weps) return;

		for (auto it = weps.begin(); it != weps.end(); ++it) {
			WeaponEntry w;
			w.name = it->first.as<std::string>();
			auto& v = it->second;
			w.index = yamlInt(v, "index");
			w.attackSound = yamlStr(v, "attack_sound");
			if (v["damage"]) {
				w.damageMin = yamlInt(v["damage"], "min");
				w.damageMax = yamlInt(v["damage"], "max");
			}
			if (v["range"]) {
				w.rangeMin = yamlInt(v["range"], "min");
				w.rangeMax = yamlInt(v["range"], "max");
			}
			if (v["ammo"]) {
				w.ammoType = yamlStr(v["ammo"], "type");
				w.ammoUsage = yamlInt(v["ammo"], "usage");
			}
			w.projectile = yamlStr(v, "projectile");
			w.shots = yamlInt(v, "shots", 1);
			w.shotHold = yamlInt(v, "shot_hold");
			weapons_.push_back(std::move(w));
		}
		std::fprintf(stderr, "Loaded %zu weapons\n", weapons_.size());
	} catch (const YAML::Exception& ex) {
		std::fprintf(stderr, "Failed to load weapons.yaml: %s\n", ex.what());
	}
}

void AssetBrowser::loadProjectiles() {
	try {
		YAML::Node root = YAML::LoadFile(gameDir_ + "/projectiles.yaml");
		YAML::Node projs = root["projectiles"];
		if (!projs) return;

		for (auto it = projs.begin(); it != projs.end(); ++it) {
			ProjectileEntry p;
			p.name = it->first.as<std::string>();
			p.launchSpeed = 0;
			auto& v = it->second;
			if (v["launch"]) {
				p.launchRenderMode = yamlStr(v["launch"], "render_mode");
				p.launchAnim = yamlStr(v["launch"], "anim");
				if (!p.launchAnim.empty()) {
					// ok
				} else {
					p.launchAnim = yamlStr(v["launch"], "anim_player");
				}
				p.launchSpeed = yamlInt(v["launch"], "speed");
			}
			if (v["impact"]) {
				p.impactAnim = yamlStr(v["impact"], "anim");
				p.impactRenderMode = yamlStr(v["impact"], "render_mode");
				p.impactSound = yamlStr(v["impact"], "impact_sound");
			}
			projectiles_.push_back(std::move(p));
		}
		std::fprintf(stderr, "Loaded %zu projectiles\n", projectiles_.size());
	} catch (const YAML::Exception& ex) {
		std::fprintf(stderr, "Failed to load projectiles.yaml: %s\n", ex.what());
	}
}

void AssetBrowser::loadCharacters() {
	try {
		YAML::Node root = YAML::LoadFile(gameDir_ + "/characters.yaml");
		YAML::Node chars = root["characters"];
		if (!chars) return;

		for (auto it = chars.begin(); it != chars.end(); ++it) {
			CharacterEntry c;
			c.name = it->first.as<std::string>();
			auto& v = it->second;
			c.defense = yamlInt(v, "defense");
			c.strength = yamlInt(v, "strength");
			c.accuracy = yamlInt(v, "accuracy");
			c.agility = yamlInt(v, "agility");
			c.iq = yamlInt(v, "iq");
			c.credits = yamlInt(v, "credits");
			characters_.push_back(std::move(c));
		}
		std::fprintf(stderr, "Loaded %zu characters\n", characters_.size());
	} catch (const YAML::Exception& ex) {
		std::fprintf(stderr, "Failed to load characters.yaml: %s\n", ex.what());
	}
}

void AssetBrowser::loadEffects() {
	try {
		YAML::Node root = YAML::LoadFile(gameDir_ + "/effects.yaml");
		YAML::Node buffsNode = root["buffs"];
		if (!buffsNode) return;

		for (auto it = buffsNode.begin(); it != buffsNode.end(); ++it) {
			BuffEntry b;
			b.name = it->first.as<std::string>();
			auto& v = it->second;
			b.maxStacks = yamlInt(v, "max_stacks", 1);
			b.hasAmount = v["has_amount"] && v["has_amount"].as<bool>();
			b.drawAmount = v["draw_amount"] && v["draw_amount"].as<bool>();
			buffs_.push_back(std::move(b));
		}
		std::fprintf(stderr, "Loaded %zu buffs/effects\n", buffs_.size());
	} catch (const YAML::Exception& ex) {
		std::fprintf(stderr, "Failed to load effects.yaml: %s\n", ex.what());
	}
}

void AssetBrowser::loadAnimations() {
	try {
		YAML::Node root = YAML::LoadFile(gameDir_ + "/sprites.yaml");
		YAML::Node tiles = root["tiles"];
		if (!tiles) return;

		for (auto it = tiles.begin(); it != tiles.end(); ++it) {
			AnimationEntry a;
			a.name = it->first.as<std::string>();
			a.tileIndex = it->second.as<int>();
			animations_.push_back(a);
			tileNameToIndex_[a.name] = a.tileIndex;
		}
		std::fprintf(stderr, "Loaded %zu animations/tiles\n", animations_.size());
	} catch (const YAML::Exception& ex) {
		std::fprintf(stderr, "Failed to load sprites.yaml: %s\n", ex.what());
	}
}

// ─── Sprite Preview Helpers ─────────────────────────────────────────────────

void AssetBrowser::drawSpritePreview(int tileIndex, float scale) {
	GLuint tex = sprites_.getTextureForTile(tileIndex);
	if (!tex) {
		ImGui::TextDisabled("(no sprite for tile %d)", tileIndex);
		return;
	}

	int w, h;
	int mediaID = tileIndex; // getTextureForTile handles mapping internally
	// We need the actual media ID to get dimensions
	sprites_.getDimensions(sprites_.getTextureForTile(tileIndex) ? tileIndex : 0, w, h);
	// Actually use tile→media mapping. SpriteLoader handles this internally,
	// but we need dimensions from the media ID. Let's just use a reasonable size.
	// The texture is already created, just display it at scaled size.

	// Query actual texture dimensions via OpenGL
	GLint texW = 0, texH = 0;
	glBindTexture(GL_TEXTURE_2D, tex);
	glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &texW);
	glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &texH);

	if (texW > 0 && texH > 0) {
		ImVec2 size(texW * scale, texH * scale);
		// Checkerboard background for transparency
		ImVec2 pos = ImGui::GetCursorScreenPos();
		ImDrawList* dl = ImGui::GetWindowDrawList();
		dl->AddRectFilled(pos, ImVec2(pos.x + size.x, pos.y + size.y),
		                  IM_COL32(40, 40, 40, 255));
		// Draw checkered pattern
		int checkSize = (int)(8 * scale);
		if (checkSize < 4) checkSize = 4;
		for (int cy = 0; cy < (int)size.y; cy += checkSize) {
			for (int cx = 0; cx < (int)size.x; cx += checkSize) {
				if (((cx / checkSize) + (cy / checkSize)) % 2 == 0) {
					float x0 = pos.x + cx;
					float y0 = pos.y + cy;
					float x1 = std::min(x0 + checkSize, pos.x + size.x);
					float y1 = std::min(y0 + checkSize, pos.y + size.y);
					dl->AddRectFilled(ImVec2(x0, y0), ImVec2(x1, y1),
					                  IM_COL32(60, 60, 60, 255));
				}
			}
		}

		ImGui::Image((ImTextureID)(intptr_t)tex, size);
		ImGui::Text("%dx%d", texW, texH);
	}
}

void AssetBrowser::drawAnimatedSprite(int tileIndex, float scale) {
	int frameCount = sprites_.getFrameCount(tileIndex);
	if (frameCount <= 1) {
		drawSpritePreview(tileIndex, scale);
		return;
	}

	// Animation controls
	ImGui::Checkbox("Play", &animPlaying_);
	ImGui::SameLine();
	ImGui::SetNextItemWidth(100);
	ImGui::SliderFloat("FPS", &animSpeed_, 1.0f, 30.0f, "%.0f");
	ImGui::SameLine();
	ImGui::Text("Frames: %d", frameCount);

	if (animPlaying_) {
		animTime_ += ImGui::GetIO().DeltaTime * animSpeed_;
	}
	int currentFrame = (int)animTime_ % frameCount;

	// Frame slider
	ImGui::SliderInt("Frame", &currentFrame, 0, frameCount - 1);
	if (!animPlaying_) {
		animTime_ = (float)currentFrame;
	}

	// Draw the current frame
	GLuint tex = sprites_.getTextureForTile(tileIndex, currentFrame);
	if (!tex) {
		ImGui::TextDisabled("(no sprite for frame %d)", currentFrame);
		return;
	}

	GLint texW = 0, texH = 0;
	glBindTexture(GL_TEXTURE_2D, tex);
	glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &texW);
	glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &texH);

	if (texW > 0 && texH > 0) {
		ImVec2 size(texW * scale, texH * scale);
		ImVec2 pos = ImGui::GetCursorScreenPos();
		ImDrawList* dl = ImGui::GetWindowDrawList();
		dl->AddRectFilled(pos, ImVec2(pos.x + size.x, pos.y + size.y),
		                  IM_COL32(40, 40, 40, 255));
		int checkSize = (int)(8 * scale);
		if (checkSize < 4) checkSize = 4;
		for (int cy = 0; cy < (int)size.y; cy += checkSize) {
			for (int cx = 0; cx < (int)size.x; cx += checkSize) {
				if (((cx / checkSize) + (cy / checkSize)) % 2 == 0) {
					float x0 = pos.x + cx;
					float y0 = pos.y + cy;
					float x1 = std::min(x0 + checkSize, pos.x + size.x);
					float y1 = std::min(y0 + checkSize, pos.y + size.y);
					dl->AddRectFilled(ImVec2(x0, y0), ImVec2(x1, y1),
					                  IM_COL32(60, 60, 60, 255));
				}
			}
		}
		ImGui::Image((ImTextureID)(intptr_t)tex, size);
	}

	// Draw all frames as a strip below
	if (frameCount > 1 && ImGui::CollapsingHeader("All Frames")) {
		float thumbScale = 2.0f;
		for (int f = 0; f < frameCount; f++) {
			GLuint ftex = sprites_.getTextureForTile(tileIndex, f);
			if (!ftex) continue;

			GLint fw = 0, fh = 0;
			glBindTexture(GL_TEXTURE_2D, ftex);
			glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &fw);
			glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &fh);

			if (fw > 0 && fh > 0) {
				if (f > 0) ImGui::SameLine();
				ImGui::BeginGroup();
				bool isCurrent = (f == currentFrame);
				if (isCurrent) {
					ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(1, 1, 0, 1));
					ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 2.0f);
				}
				ImGui::Image((ImTextureID)(intptr_t)ftex,
				             ImVec2(fw * thumbScale, fh * thumbScale));
				if (isCurrent) {
					ImGui::PopStyleVar();
					ImGui::PopStyleColor();
				}
				ImGui::Text("%d", f);
				ImGui::EndGroup();
			}
		}
	}
}

void AssetBrowser::drawCompositeMonster(int tileIndex,
                                         const SpriteLoader::BodyPartOffsets& offsets,
                                         bool isFloater, bool isSpecialBoss, float scale) {
	// For floaters and special bosses, just show the raw animated sprite for now
	// (they have different composition rules — simple/multipart/spider/ethereal)
	if (isFloater || isSpecialBoss) {
		drawAnimatedSprite(tileIndex, scale);
		return;
	}

	int totalFrames = sprites_.getFrameCount(tileIndex);
	// Need at least 4 frames for body part composition (legs, walk, torso, head)
	if (totalFrames < 4) {
		drawAnimatedSprite(tileIndex, scale);
		return;
	}

	// Pose selector
	static const char* poseNames[] = {
		"Idle (front)", "Walk (front)", "Attack 1", "Attack 2",
		"Idle (back)", "Walk (back)"
	};
	int maxPose = (totalFrames >= 8) ? 5 : 1;
	if (totalFrames >= 12) maxPose = 5; // has attack frames

	ImGui::Text("Pose:");
	ImGui::SameLine();
	for (int p = 0; p <= maxPose; p++) {
		if (p > 0) ImGui::SameLine();
		bool active = (selectedPose_ == p);
		if (active) ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.4f, 0.6f, 0.9f, 1.0f));
		if (ImGui::SmallButton(poseNames[p])) {
			selectedPose_ = p;
			animTime_ = 0.0f;
		}
		if (active) ImGui::PopStyleColor();
	}

	// Walk/attack frame animation
	int walkFrame = 0;
	bool isWalkOrAttack = (selectedPose_ == 1 || selectedPose_ == 2 ||
	                       selectedPose_ == 3 || selectedPose_ == 5);
	if (isWalkOrAttack) {
		ImGui::SameLine();
		ImGui::Checkbox("Animate", &animPlaying_);
		if (animPlaying_) {
			animTime_ += ImGui::GetIO().DeltaTime * animSpeed_;
			walkFrame = (int)animTime_ % 2;
		}
	}

	// Build the composite
	SpriteLoader::BodyPartOffsets poseOffsets = offsets;
	GLuint tex = sprites_.getCompositePose(tileIndex, selectedPose_, poseOffsets, walkFrame);

	if (!tex) {
		ImGui::TextDisabled("(no composite for tile %d)", tileIndex);
		return;
	}

	// Query composite texture size
	GLint texW = 0, texH = 0;
	glBindTexture(GL_TEXTURE_2D, tex);
	glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &texW);
	glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &texH);

	if (texW > 0 && texH > 0) {
		ImVec2 size(texW * scale, texH * scale);
		// Checkerboard background
		ImVec2 pos = ImGui::GetCursorScreenPos();
		ImDrawList* dl = ImGui::GetWindowDrawList();
		dl->AddRectFilled(pos, ImVec2(pos.x + size.x, pos.y + size.y),
		                  IM_COL32(40, 40, 40, 255));
		int checkSize = (int)(8 * scale);
		if (checkSize < 4) checkSize = 4;
		for (int cy = 0; cy < (int)size.y; cy += checkSize) {
			for (int cx = 0; cx < (int)size.x; cx += checkSize) {
				if (((cx / checkSize) + (cy / checkSize)) % 2 == 0) {
					float x0 = pos.x + cx;
					float y0 = pos.y + cy;
					float x1 = std::min(x0 + checkSize, pos.x + size.x);
					float y1 = std::min(y0 + checkSize, pos.y + size.y);
					dl->AddRectFilled(ImVec2(x0, y0), ImVec2(x1, y1),
					                  IM_COL32(60, 60, 60, 255));
				}
			}
		}
		ImGui::Image((ImTextureID)(intptr_t)tex, size);
		ImGui::Text("Composite: %dx%d", texW, texH);
	}

	// Show individual body part frames below
	if (ImGui::CollapsingHeader("Individual Frames")) {
		float thumbScale = 2.0f;
		int count = sprites_.getFrameCount(tileIndex);
		static const char* frameLabels[] = {
			"Legs", "Legs Walk", "Torso", "Head",
			"Legs Back", "Legs Walk Back", "Torso Back", "Head Back",
			"Atk1 F0", "Atk1 F1", "Atk2 F0", "Atk2 F1",
			"Pain", "Death"
		};
		for (int f = 0; f < count; f++) {
			GLuint ftex = sprites_.getTextureForTile(tileIndex, f);
			if (!ftex) continue;

			GLint fw = 0, fh = 0;
			glBindTexture(GL_TEXTURE_2D, ftex);
			glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &fw);
			glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &fh);

			if (fw > 0 && fh > 0) {
				ImGui::BeginGroup();
				ImGui::Image((ImTextureID)(intptr_t)ftex,
				             ImVec2(fw * thumbScale, fh * thumbScale));
				if (f < 14) ImGui::Text("%s", frameLabels[f]);
				else ImGui::Text("F%d", f);
				ImGui::EndGroup();
				if (f < count - 1) ImGui::SameLine();
			}
		}
	}
}

// ─── Drawing ────────────────────────────────────────────────────────────────

void AssetBrowser::draw() {
	if (!loaded_) return;

	// Full-window dockspace
	ImGuiViewport* viewport = ImGui::GetMainViewport();
	ImGui::SetNextWindowPos(viewport->WorkPos);
	ImGui::SetNextWindowSize(viewport->WorkSize);
	ImGui::Begin("Assets Manager", nullptr,
		ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
		ImGuiWindowFlags_NoBringToFrontOnFocus);

	// Title bar
	ImGui::Text("DoomIIRPG Assets Manager");
	ImGui::Separator();

	// Filter
	ImGui::SetNextItemWidth(300);
	ImGui::InputText("Filter", filterText_, sizeof(filterText_));
	ImGui::SameLine();
	if (ImGui::Button("Clear")) filterText_[0] = '\0';
	ImGui::SameLine();
	ImGui::TextDisabled("(%zu entities, %zu monsters, %zu weapons)",
		entities_.size(), monsters_.size(), weapons_.size());

	ImGui::Separator();

	drawCategoryTabs();

	ImGui::End();
}

void AssetBrowser::drawCategoryTabs() {
	if (ImGui::BeginTabBar("AssetTabs")) {
		if (ImGui::BeginTabItem("Entities")) {
			currentCategory_ = AssetCategory::Entities;
			drawEntitiesPanel();
			ImGui::EndTabItem();
		}
		if (ImGui::BeginTabItem("Monsters")) {
			currentCategory_ = AssetCategory::Monsters;
			drawMonstersPanel();
			ImGui::EndTabItem();
		}
		if (ImGui::BeginTabItem("Weapons")) {
			currentCategory_ = AssetCategory::Weapons;
			drawWeaponsPanel();
			ImGui::EndTabItem();
		}
		if (ImGui::BeginTabItem("Projectiles")) {
			currentCategory_ = AssetCategory::Projectiles;
			drawProjectilesPanel();
			ImGui::EndTabItem();
		}
		if (ImGui::BeginTabItem("Characters")) {
			currentCategory_ = AssetCategory::Characters;
			drawCharactersPanel();
			ImGui::EndTabItem();
		}
		if (ImGui::BeginTabItem("Effects")) {
			currentCategory_ = AssetCategory::Effects;
			drawEffectsPanel();
			ImGui::EndTabItem();
		}
		if (ImGui::BeginTabItem("Animations")) {
			currentCategory_ = AssetCategory::Animations;
			drawAnimationsPanel();
			ImGui::EndTabItem();
		}
		ImGui::EndTabBar();
	}
}

// ─── Entities Panel ─────────────────────────────────────────────────────────

static ImVec4 entityTypeColor(const std::string& type) {
	if (type == "monster")    return ImVec4(1.0f, 0.3f, 0.3f, 1.0f);
	if (type == "item")       return ImVec4(0.3f, 1.0f, 0.3f, 1.0f);
	if (type == "npc")        return ImVec4(0.3f, 0.7f, 1.0f, 1.0f);
	if (type == "door")       return ImVec4(1.0f, 0.8f, 0.2f, 1.0f);
	if (type == "decor")      return ImVec4(0.7f, 0.7f, 0.7f, 1.0f);
	if (type == "corpse")     return ImVec4(0.5f, 0.4f, 0.3f, 1.0f);
	if (type == "spritewall") return ImVec4(0.6f, 0.5f, 0.8f, 1.0f);
	return ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
}

void AssetBrowser::drawEntitiesPanel() {
	// Left: list  Right: detail
	float listWidth = 350.0f;

	ImGui::BeginChild("EntityList", ImVec2(listWidth, 0), true);
	for (int i = 0; i < (int)entities_.size(); i++) {
		auto& e = entities_[i];
		if (!matchesFilter(e.name, filterText_) && !matchesFilter(e.type, filterText_))
			continue;

		// Sprite thumbnail
		if (e.tileIndex > 0) {
			GLuint thumb = sprites_.getTextureForTile(e.tileIndex);
			if (thumb) {
				ImGui::Image((ImTextureID)(intptr_t)thumb, ImVec2(16, 16));
				ImGui::SameLine();
			}
		}

		ImGui::PushStyleColor(ImGuiCol_Text, entityTypeColor(e.type));
		bool selected = (selectedEntity_ == i);
		if (ImGui::Selectable(e.name.c_str(), selected)) {
			selectedEntity_ = i;
			animTime_ = 0.0f;
		}
		ImGui::PopStyleColor();

		// Type badge on same line
		ImGui::SameLine(listWidth - 80);
		ImGui::TextDisabled("[%s]", e.type.c_str());
	}
	ImGui::EndChild();

	ImGui::SameLine();

	ImGui::BeginChild("EntityDetail", ImVec2(0, 0), true);
	if (selectedEntity_ >= 0 && selectedEntity_ < (int)entities_.size()) {
		drawEntityDetail(entities_[selectedEntity_]);
	} else {
		ImGui::TextDisabled("Select an entity from the list");
	}
	ImGui::EndChild();
}

void AssetBrowser::drawEntityDetail(const EntityEntry& e) {
	ImGui::PushStyleColor(ImGuiCol_Text, entityTypeColor(e.type));
	ImGui::Text("%s", e.name.c_str());
	ImGui::PopStyleColor();
	ImGui::Separator();

	// Sprite preview — use composite for monsters, simple for others
	if (e.tileIndex > 0) {
		if (e.type == "monster") {
			drawCompositeMonster(e.tileIndex, e.bodyParts, e.isFloater, e.isSpecialBoss, 4.0f);
		} else {
			drawAnimatedSprite(e.tileIndex, 4.0f);
		}
		ImGui::Separator();
	}

	if (ImGui::BeginTable("EntityProps", 2, ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerH)) {
		ImGui::TableSetupColumn("Property", ImGuiTableColumnFlags_WidthFixed, 120);
		ImGui::TableSetupColumn("Value");

		auto row = [](const char* label, const char* fmt, ...) {
			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			ImGui::TextDisabled("%s", label);
			ImGui::TableNextColumn();
			va_list args;
			va_start(args, fmt);
			ImGui::TextV(fmt, args);
			va_end(args);
		};

		row("Type", "%s", e.type.c_str());
		row("Subtype", "%s", e.subtype.c_str());
		row("Tile Index", "%d", e.tileIndex);
		row("Parm", "%d", e.parm);
		row("Name Str", "%d", e.nameStr);
		row("Long Name Str", "%d", e.longNameStr);
		row("Description Str", "%d", e.descStr);

		ImGui::EndTable();
	}
}

// ─── Monsters Panel ─────────────────────────────────────────────────────────

void AssetBrowser::drawMonstersPanel() {
	float listWidth = 250.0f;

	ImGui::BeginChild("MonsterList", ImVec2(listWidth, 0), true);
	for (int i = 0; i < (int)monsters_.size(); i++) {
		auto& m = monsters_[i];
		if (!matchesFilter(m.name, filterText_)) continue;

		// Composite thumbnail
		std::string tileName = "monster_" + m.name;
		auto tileIt = tileNameToIndex_.find(tileName);
		if (tileIt != tileNameToIndex_.end()) {
			// Find entity body parts for this monster
			SpriteLoader::BodyPartOffsets offsets;
			for (const auto& ent : entities_) {
				if (ent.tileIndex == tileIt->second && ent.type == "monster") {
					offsets = ent.bodyParts;
					break;
				}
			}
			GLuint thumb = sprites_.getCompositeMonster(tileIt->second, offsets);
			if (thumb) {
				ImGui::Image((ImTextureID)(intptr_t)thumb, ImVec2(16, 24));
				ImGui::SameLine();
			}
		}

		bool selected = (selectedMonster_ == i);
		if (ImGui::Selectable(m.name.c_str(), selected)) {
			selectedMonster_ = i;
			animTime_ = 0.0f;
		}
		ImGui::SameLine(listWidth - 60);
		ImGui::TextDisabled("%d tiers", (int)m.tiers.size());
	}
	ImGui::EndChild();

	ImGui::SameLine();

	ImGui::BeginChild("MonsterDetail", ImVec2(0, 0), true);
	if (selectedMonster_ >= 0 && selectedMonster_ < (int)monsters_.size()) {
		drawMonsterDetail(monsters_[selectedMonster_]);
	} else {
		ImGui::TextDisabled("Select a monster from the list");
	}
	ImGui::EndChild();
}

void AssetBrowser::drawMonsterDetail(const MonsterEntry& m) {
	ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "%s", m.name.c_str());
	ImGui::SameLine();
	ImGui::TextDisabled("(index: %d)", m.index);
	ImGui::Separator();

	// Monster sprite preview — composite body parts
	std::string tileName = "monster_" + m.name;
	auto tileIt = tileNameToIndex_.find(tileName);
	if (tileIt != tileNameToIndex_.end()) {
		// Find the matching entity to get body part offsets
		SpriteLoader::BodyPartOffsets offsets;
		bool isFloater = false, isSpecialBoss = false;
		for (const auto& ent : entities_) {
			if (ent.tileIndex == tileIt->second && ent.type == "monster") {
				offsets = ent.bodyParts;
				isFloater = ent.isFloater;
				isSpecialBoss = ent.isSpecialBoss;
				break;
			}
		}
		drawCompositeMonster(tileIt->second, offsets, isFloater, isSpecialBoss, 4.0f);
		ImGui::Separator();
	}

	// Blood color swatch
	if (!m.bloodColor.empty() && m.bloodColor[0] == '#' && m.bloodColor.size() >= 7) {
		unsigned int r, g, b;
		std::sscanf(m.bloodColor.c_str() + 1, "%02x%02x%02x", &r, &g, &b);
		ImVec4 col(r / 255.0f, g / 255.0f, b / 255.0f, 1.0f);
		ImGui::ColorButton("Blood", col, ImGuiColorEditFlags_NoTooltip, ImVec2(20, 20));
		ImGui::SameLine();
		ImGui::Text("Blood: %s", m.bloodColor.c_str());
	}

	// Sounds
	if (ImGui::CollapsingHeader("Sounds", ImGuiTreeNodeFlags_DefaultOpen)) {
		if (ImGui::BeginTable("MonsterSounds", 2, ImGuiTableFlags_RowBg)) {
			ImGui::TableSetupColumn("Event", ImGuiTableColumnFlags_WidthFixed, 80);
			ImGui::TableSetupColumn("Sound");

			auto sndRow = [](const char* label, const std::string& snd) {
				if (snd.empty()) return;
				ImGui::TableNextRow();
				ImGui::TableNextColumn(); ImGui::TextDisabled("%s", label);
				ImGui::TableNextColumn(); ImGui::Text("%s", snd.c_str());
			};

			sndRow("Alert 1", m.alertSnd1);
			sndRow("Alert 2", m.alertSnd2);
			sndRow("Alert 3", m.alertSnd3);
			sndRow("Attack 1", m.attackSnd1);
			sndRow("Attack 2", m.attackSnd2);
			sndRow("Idle", m.idleSnd);
			sndRow("Pain", m.painSnd);
			sndRow("Death", m.deathSnd);

			ImGui::EndTable();
		}
	}

	// Tiers
	for (int ti = 0; ti < (int)m.tiers.size(); ti++) {
		auto& t = m.tiers[ti];
		char label[32];
		std::snprintf(label, sizeof(label), "Tier %d (parm %d)", ti, ti);
		if (ImGui::CollapsingHeader(label, ImGuiTreeNodeFlags_DefaultOpen)) {
			// Stats bar visualization
			ImGui::Text("Stats:");
			auto statBar = [](const char* name, int val, int maxVal, ImVec4 color) {
				ImGui::Text("  %-10s %3d", name, val);
				ImGui::SameLine(160);
				float frac = (maxVal > 0) ? (float)val / (float)maxVal : 0.0f;
				ImGui::PushStyleColor(ImGuiCol_PlotHistogram, color);
				ImGui::ProgressBar(frac, ImVec2(200, 14), "");
				ImGui::PopStyleColor();
			};

			statBar("Health", t.health, 500, ImVec4(0.8f, 0.2f, 0.2f, 1.0f));
			statBar("Armor", t.armor, 100, ImVec4(0.3f, 0.3f, 0.8f, 1.0f));
			statBar("Defense", t.defense, 100, ImVec4(0.3f, 0.6f, 0.9f, 1.0f));
			statBar("Strength", t.strength, 200, ImVec4(0.9f, 0.5f, 0.1f, 1.0f));
			statBar("Accuracy", t.accuracy, 100, ImVec4(0.9f, 0.9f, 0.2f, 1.0f));
			statBar("Agility", t.agility, 100, ImVec4(0.2f, 0.8f, 0.2f, 1.0f));

			// Attacks
			ImGui::Spacing();
			ImGui::Text("Attacks:");
			ImGui::Text("  Primary:   %s", t.attack1.c_str());
			ImGui::Text("  Secondary: %s", t.attack2.c_str());
			if (t.chance > 0)
				ImGui::Text("  Chance:    %d%%", t.chance);

			// Weaknesses
			if (!t.weaknesses.empty()) {
				ImGui::Spacing();
				ImGui::Text("Weaknesses:");
				for (auto& [wep, dmg] : t.weaknesses) {
					ImVec4 col = (dmg > 100) ? ImVec4(0.2f, 1.0f, 0.2f, 1.0f) :
					             (dmg < 100) ? ImVec4(1.0f, 0.4f, 0.4f, 1.0f) :
					                           ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
					ImGui::TextColored(col, "  %-20s %.0f%%", wep.c_str(), dmg);
				}
			}
		}
	}
}

// ─── Weapons Panel ──────────────────────────────────────────────────────────

void AssetBrowser::drawWeaponsPanel() {
	float listWidth = 250.0f;

	ImGui::BeginChild("WeaponList", ImVec2(listWidth, 0), true);
	for (int i = 0; i < (int)weapons_.size(); i++) {
		auto& w = weapons_[i];
		if (!matchesFilter(w.name, filterText_)) continue;

		// Sprite thumbnail
		auto tileIt = tileNameToIndex_.find(w.name);
		if (tileIt != tileNameToIndex_.end()) {
			GLuint thumb = sprites_.getTextureForTile(tileIt->second);
			if (thumb) {
				ImGui::Image((ImTextureID)(intptr_t)thumb, ImVec2(16, 16));
				ImGui::SameLine();
			}
		}

		bool selected = (selectedWeapon_ == i);
		if (ImGui::Selectable(w.name.c_str(), selected)) {
			selectedWeapon_ = i;
			animTime_ = 0.0f;
		}
	}
	ImGui::EndChild();

	ImGui::SameLine();

	ImGui::BeginChild("WeaponDetail", ImVec2(0, 0), true);
	if (selectedWeapon_ >= 0 && selectedWeapon_ < (int)weapons_.size()) {
		drawWeaponDetail(weapons_[selectedWeapon_]);
	} else {
		ImGui::TextDisabled("Select a weapon from the list");
	}
	ImGui::EndChild();
}

void AssetBrowser::drawWeaponDetail(const WeaponEntry& w) {
	ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "%s", w.name.c_str());
	ImGui::SameLine();
	ImGui::TextDisabled("(index: %d)", w.index);
	ImGui::Separator();

	// Weapon sprite preview — look up tile index from sprites.yaml
	auto tileIt = tileNameToIndex_.find(w.name);
	if (tileIt != tileNameToIndex_.end()) {
		drawAnimatedSprite(tileIt->second, 4.0f);
		ImGui::Separator();
	}

	if (ImGui::BeginTable("WeaponProps", 2, ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerH)) {
		ImGui::TableSetupColumn("Property", ImGuiTableColumnFlags_WidthFixed, 120);
		ImGui::TableSetupColumn("Value");

		auto row = [](const char* label, const char* fmt, ...) {
			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			ImGui::TextDisabled("%s", label);
			ImGui::TableNextColumn();
			va_list args;
			va_start(args, fmt);
			ImGui::TextV(fmt, args);
			va_end(args);
		};

		row("Damage", "%d - %d", w.damageMin, w.damageMax);
		row("Range", "%d - %d", w.rangeMin, w.rangeMax);
		row("Ammo Type", "%s", w.ammoType.c_str());
		row("Ammo Usage", "%d", w.ammoUsage);
		row("Projectile", "%s", w.projectile.c_str());
		row("Shots", "%d", w.shots);
		row("Shot Hold", "%d ms", w.shotHold);
		row("Attack Sound", "%s", w.attackSound.c_str());

		ImGui::EndTable();
	}

	// DPS estimate
	ImGui::Spacing();
	float avgDmg = (w.damageMin + w.damageMax) / 2.0f;
	float dps = (w.shotHold > 0) ? (avgDmg * w.shots * 1000.0f / w.shotHold) : 0;
	ImGui::Text("Avg damage per shot: %.1f", avgDmg);
	ImGui::Text("Theoretical DPS: %.1f", dps);
}

// ─── Projectiles Panel ──────────────────────────────────────────────────────

void AssetBrowser::drawProjectilesPanel() {
	if (ImGui::BeginTable("Projectiles", 6,
		ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders | ImGuiTableFlags_Resizable |
		ImGuiTableFlags_ScrollY)) {
		ImGui::TableSetupScrollFreeze(0, 1);
		ImGui::TableSetupColumn("Name");
		ImGui::TableSetupColumn("Launch Mode");
		ImGui::TableSetupColumn("Launch Anim");
		ImGui::TableSetupColumn("Speed");
		ImGui::TableSetupColumn("Impact Anim");
		ImGui::TableSetupColumn("Impact Sound");
		ImGui::TableHeadersRow();

		for (auto& p : projectiles_) {
			if (!matchesFilter(p.name, filterText_)) continue;

			ImGui::TableNextRow();
			ImGui::TableNextColumn(); ImGui::Text("%s", p.name.c_str());
			ImGui::TableNextColumn(); ImGui::Text("%s", p.launchRenderMode.c_str());
			ImGui::TableNextColumn(); ImGui::Text("%s", p.launchAnim.c_str());
			ImGui::TableNextColumn();
			if (p.launchSpeed > 0) ImGui::Text("%d", p.launchSpeed);
			else ImGui::TextDisabled("-");
			ImGui::TableNextColumn(); ImGui::Text("%s", p.impactAnim.c_str());
			ImGui::TableNextColumn(); ImGui::Text("%s", p.impactSound.c_str());
		}
		ImGui::EndTable();
	}
}

// ─── Characters Panel ───────────────────────────────────────────────────────

void AssetBrowser::drawCharactersPanel() {
	for (int i = 0; i < (int)characters_.size(); i++) {
		auto& c = characters_[i];
		if (!matchesFilter(c.name, filterText_)) continue;

		ImGui::PushID(i);
		if (ImGui::CollapsingHeader(c.name.c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
			auto statBar = [](const char* name, int val, int maxVal, ImVec4 color) {
				ImGui::Text("  %-10s %3d", name, val);
				ImGui::SameLine(160);
				float frac = (maxVal > 0) ? (float)val / (float)maxVal : 0.0f;
				ImGui::PushStyleColor(ImGuiCol_PlotHistogram, color);
				ImGui::ProgressBar(frac, ImVec2(200, 14), "");
				ImGui::PopStyleColor();
			};

			statBar("Defense", c.defense, 20, ImVec4(0.3f, 0.6f, 0.9f, 1.0f));
			statBar("Strength", c.strength, 20, ImVec4(0.9f, 0.5f, 0.1f, 1.0f));
			statBar("Accuracy", c.accuracy, 100, ImVec4(0.9f, 0.9f, 0.2f, 1.0f));
			statBar("Agility", c.agility, 20, ImVec4(0.2f, 0.8f, 0.2f, 1.0f));
			statBar("IQ", c.iq, 200, ImVec4(0.7f, 0.3f, 0.9f, 1.0f));
			ImGui::Text("  Credits:   %d", c.credits);
		}
		ImGui::PopID();
	}
}

// ─── Effects Panel ──────────────────────────────────────────────────────────

void AssetBrowser::drawEffectsPanel() {
	if (ImGui::BeginTable("Buffs", 4,
		ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders | ImGuiTableFlags_Resizable)) {
		ImGui::TableSetupColumn("Name");
		ImGui::TableSetupColumn("Max Stacks");
		ImGui::TableSetupColumn("Has Amount");
		ImGui::TableSetupColumn("Draw Amount");
		ImGui::TableHeadersRow();

		for (auto& b : buffs_) {
			if (!matchesFilter(b.name, filterText_)) continue;

			ImGui::TableNextRow();
			ImGui::TableNextColumn(); ImGui::Text("%s", b.name.c_str());
			ImGui::TableNextColumn(); ImGui::Text("%d", b.maxStacks);
			ImGui::TableNextColumn();
			ImGui::TextColored(b.hasAmount ? ImVec4(0.2f, 0.8f, 0.2f, 1.0f) : ImVec4(0.5f, 0.5f, 0.5f, 1.0f),
				"%s", b.hasAmount ? "Yes" : "No");
			ImGui::TableNextColumn();
			ImGui::TextColored(b.drawAmount ? ImVec4(0.2f, 0.8f, 0.2f, 1.0f) : ImVec4(0.5f, 0.5f, 0.5f, 1.0f),
				"%s", b.drawAmount ? "Yes" : "No");
		}
		ImGui::EndTable();
	}
}

// ─── Animations Panel ───────────────────────────────────────────────────────

void AssetBrowser::drawAnimationsPanel() {
	float listWidth = 300.0f;

	ImGui::BeginChild("AnimList", ImVec2(listWidth, 0), true);
	for (int i = 0; i < (int)animations_.size(); i++) {
		auto& a = animations_[i];
		if (!matchesFilter(a.name, filterText_)) continue;

		bool selected = (selectedAnimation_ == i);
		int frames = sprites_.getFrameCount(a.tileIndex);

		// Show a small sprite thumbnail in the list
		GLuint thumb = sprites_.getTextureForTile(a.tileIndex);
		if (thumb) {
			ImGui::Image((ImTextureID)(intptr_t)thumb, ImVec2(16, 16));
			ImGui::SameLine();
		}

		if (ImGui::Selectable(a.name.c_str(), selected)) {
			selectedAnimation_ = i;
			animTime_ = 0.0f;
		}
		ImGui::SameLine(listWidth - 70);
		ImGui::TextDisabled("%d (%df)", a.tileIndex, frames);
	}
	ImGui::EndChild();

	ImGui::SameLine();

	ImGui::BeginChild("AnimDetail", ImVec2(0, 0), true);
	if (selectedAnimation_ >= 0 && selectedAnimation_ < (int)animations_.size()) {
		auto& a = animations_[selectedAnimation_];
		ImGui::Text("%s", a.name.c_str());
		ImGui::SameLine();
		ImGui::TextDisabled("(tile: %d, frames: %d)", a.tileIndex,
		                    sprites_.getFrameCount(a.tileIndex));
		ImGui::Separator();
		drawAnimatedSprite(a.tileIndex, 4.0f);
	} else {
		ImGui::TextDisabled("Select an animation/tile from the list");
	}
	ImGui::EndChild();
}
