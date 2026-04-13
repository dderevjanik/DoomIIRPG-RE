#pragma once

struct SDL_Window;
union SDL_Event;

// Developer console overlay using Dear ImGui.
// Toggle with F12 or backtick (`) key.
class DevConsole {
public:
	DevConsole();
	~DevConsole();

	// Initialize ImGui context and backends. Call after SDL window + GL context are created.
	void init(SDL_Window* window, void* glContext);

	// Shut down ImGui backends and destroy context.
	void shutdown();

	// Process an SDL event. Returns true if ImGui consumed it (game should ignore it).
	bool processEvent(const SDL_Event& event);

	// Begin a new ImGui frame. Call once per frame before render().
	void newFrame();

	// Draw all console windows and submit ImGui draw data via OpenGL.
	void render();

	bool isVisible() const { return visible; }
	void toggle() { visible = !visible; }

private:
	bool initialized = false;
	bool visible = false;
	bool showDemo = false;

	// Frame time history for graph
	static constexpr int FRAME_HISTORY_SIZE = 120;
	float frameTimeHistory[FRAME_HISTORY_SIZE] = {};
	int frameTimeOffset = 0;

	// Sub-windows
	void drawEntityInspector();
	void drawScriptState();
	void drawGameState();
	void drawPerformance();
	void drawRenderStats();
	void drawMods();

	// Helpers
	static const char* stateName(int state);
};
