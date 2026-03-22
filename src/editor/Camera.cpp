#include "Camera.h"

#include <algorithm>

static constexpr float DEG_TO_RAD = 3.14159265358979323846f / 180.0f;

void Camera::fitToMap() {
    // Start near the center of a 32-tile map, at eye height
    constexpr float mapWorldSize = static_cast<float>(32 * 64) / 128.0f;
    posX = mapWorldSize / 2.0f;
    posY = 0.3f;  // eye height
    posZ = mapWorldSize / 2.0f;
    yaw = 0.0f;
    pitch = 0.0f;
}

void Camera::handleMouseMotion(int dx, int dy) {
    yaw -= dx * mouseSensitivity;
    pitch += dy * mouseSensitivity;
    pitch = std::clamp(pitch, -89.0f, 89.0f);
}

void Camera::handleMouseWheel(float delta) {
    moveSpeed *= (delta > 0) ? 1.15f : 0.85f;
    moveSpeed = std::clamp(moveSpeed, 1.0f, 100.0f);
}

void Camera::handleKeyboard(bool forward, bool backward, bool left, bool right,
                             bool up, bool down, bool shift, float dt) {
    float speed = moveSpeed * (shift ? 5.0f : 1.0f) * dt;
    float yawRad = yaw * DEG_TO_RAD;
    float cosY = std::cos(yawRad);
    float sinY = std::sin(yawRad);

    // Forward along the engine's actual look direction
    float fwdX = cosY;
    float fwdZ = -sinY;
    // Right perpendicular (90° clockwise)
    float rgtX = sinY;
    float rgtZ = cosY;

    if (forward)  { posX += fwdX * speed; posZ += fwdZ * speed; }
    if (backward) { posX -= fwdX * speed; posZ -= fwdZ * speed; }
    if (right)    { posX += rgtX * speed; posZ += rgtZ * speed; }
    if (left)     { posX -= rgtX * speed; posZ -= rgtZ * speed; }
    if (up)       { posY += speed; }
    if (down)     { posY -= speed; }
}

void Camera::getEngineView(int& viewX, int& viewY, int& viewZ,
                           int& viewAngle, int& viewPitch, int& viewFov) const {
    // Game calls render() with (canvas->viewX << 4) + 8
    // canvas->viewX is in tile*64 scale (e.g. tile 4 center = 4*64+32 = 288)
    // So render coords = tile*64 * 16 + 8 = tile * 1024 + 520
    int canvasX = static_cast<int>(posX * 128.0f);
    int canvasY = static_cast<int>(posZ * 128.0f);
    int canvasZ = static_cast<int>(posY * 128.0f);
    viewX = (canvasX << 4) + 8;
    viewY = (canvasY << 4) + 8;  // editor Z → engine Y (horizontal)
    viewZ = (canvasZ << 4) + 8;  // editor Y → engine Z (vertical)

    // Engine angles: 0-1023 range, 256 = 90 degrees
    viewAngle = static_cast<int>((yaw / 360.0f) * 1024.0f) & 0x3FF;
    viewPitch = static_cast<int>((-pitch / 360.0f) * 1024.0f) & 0x3FF;
    viewFov = 256;
}
