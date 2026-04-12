#pragma once
class Applet;
class DataNode;

// Game-specific data parsers registered with ResourceManager by DoomIIRPGGame.
bool parseTables(Applet* app, const DataNode& config);
bool parseSpriteAnims(Applet* app, const DataNode& config);
bool parseWeapons(Applet* app, const DataNode& config);
bool parseProjectiles(Applet* app, const DataNode& config);
bool parseEffects(Applet* app, const DataNode& config);
bool parseDialogStyles(Applet* app, const DataNode& config);
bool parseItems(Applet* app, const DataNode& config, const DataNode& effectsConfig);
bool parseMonsters(Applet* app, const DataNode& config);
bool parseMonsterCombatFromEntities(Applet* app, const DataNode& config);
