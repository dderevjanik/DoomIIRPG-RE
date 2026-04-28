#pragma once
#include <expected>
#include <string>
class Applet;
class DataNode;

// Game-specific data parsers registered with ResourceManager by DoomIIRPGGame.
std::expected<void, std::string> parseTables(Applet* app, const DataNode& config);
std::expected<void, std::string> parseSpriteAnims(Applet* app, const DataNode& config);
std::expected<void, std::string> parseWeapons(Applet* app, const DataNode& config);
std::expected<void, std::string> parseProjectiles(Applet* app, const DataNode& config);
std::expected<void, std::string> parseParticles(Applet* app, const DataNode& config);
std::expected<void, std::string> parseControls(Applet* app, const DataNode& config);
std::expected<void, std::string> parseEffects(Applet* app, const DataNode& config);
std::expected<void, std::string> parseDialogStyles(Applet* app, const DataNode& config);
std::expected<void, std::string> parseItems(Applet* app, const DataNode& config, const DataNode& effectsConfig);
std::expected<void, std::string> parseMonsters(Applet* app, const DataNode& config);
std::expected<void, std::string> parseMonsterCombatFromEntities(Applet* app, const DataNode& config);
