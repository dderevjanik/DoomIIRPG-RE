#ifndef __DOOM2RPGGAME_H__
#define __DOOM2RPGGAME_H__

#include "IGameModule.h"

// Default game module: the original Doom II RPG.
// Creates and manages all Doom II RPG game objects.
class DoomIIRPGGame : public IGameModule {
public:
	DoomIIRPGGame() {}
	~DoomIIRPGGame() override {}

	void createGameObjects(Applet* app) override;
	bool startup(Applet* app) override;
	void loadConfig(Applet* app) override;
	void shutdown(Applet* app) override;
	void registerOpcodes(Applet* app) override;
	const char* getName() const override { return "Doom II RPG"; }
};

#endif
