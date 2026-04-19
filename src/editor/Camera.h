#pragma once

#include <cmath>

namespace editor {

// Free-fly camera for the map editor (revived from git show 7a94d17~1:src/editor/Camera.h).
// Internally uses floats; getEngineView() converts to the engine's fixed-point
// view-space for Render::render().
struct Camera {
	float posX = 0.0f;
	float posY = 0.0f;  // vertical (up)
	float posZ = 0.0f;
	float yaw = 0.0f;    // degrees, 0 = +Z direction
	float pitch = 0.0f;  // degrees, positive = look up
	float moveSpeed = 3.75f;
	float mouseSensitivity = 0.2f;

	void fitToMap();
	void handleMouseMotion(int dx, int dy);
	void handleMouseWheel(float delta);
	void handleKeyboard(bool forward, bool backward, bool left, bool right,
	                    bool up, bool down, bool shift, float dt);

	void getEngineView(int& viewX, int& viewY, int& viewZ,
	                   int& viewAngle, int& viewPitch, int& viewFov) const;
};

}  // namespace editor
