#pragma once

#include <cmath>

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

    // Convert camera to engine's coordinate space for Render::render()
    void getEngineView(int& viewX, int& viewY, int& viewZ,
                       int& viewAngle, int& viewPitch, int& viewFov) const;
};
