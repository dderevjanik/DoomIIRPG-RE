#ifndef __DOOM2RPGGAME_H__
#define __DOOM2RPGGAME_H__

#include "IGameModule.h"
#include "LogoState.h"
#include "CreditsState.h"
#include "AutomapState.h"
#include "IntroMovieState.h"
#include "IntroState.h"
#include "EpilogueState.h"
#include "CharacterSelectionState.h"
#include "TravelMapState.h"
#include "DyingState.h"
#include "BotDyingState.h"
#include "ErrorState.h"
#include "BenchmarkState.h"

// Default game module: D2RPG.
// Creates and manages all D2RPG game objects.
class DoomIIRPGGame : public IGameModule {
public:
	// Canvas state handlers (owned by this module)
	LogoState logoState;
	CreditsState creditsState;
	AutomapState automapState;
	IntroMovieState introMovieState;
	IntroState introState;
	EpilogueState epilogueState;
	CharacterSelectionState characterSelectionState;
	TravelMapState travelMapState;
	DyingState dyingState;
	BotDyingState botDyingState;
	ErrorState errorState;
	BenchmarkState benchmarkState;

	DoomIIRPGGame() {}
	~DoomIIRPGGame() override {}

	void createGameObjects(Applet* app) override;
	bool startup(Applet* app) override;
	void loadConfig(Applet* app) override;
	void shutdown(Applet* app) override;
	void registerLoaders(class ResourceManager* rm) override;
	void registerOpcodes(Applet* app) override;
	void registerEventListeners(Applet* app) override;
	const char* getName() const override { return "D2RPG"; }
};

#endif
