#include "InputManager.h"
#include "Constants.h"
#include <cmath>
#include <algorithm>

namespace HeavyWeapon {

InputState InputManager::sState;
SDL_GameController* InputManager::sController = nullptr;
bool InputManager::sPrevFireCannon = false;
bool InputManager::sPrevFireNuke = false;
bool InputManager::sPrevFireMegalaser = false;
bool InputManager::sPrevPause = false;
bool InputManager::sPrevConfirm = false;
bool InputManager::sPrevCancel = false;
bool InputManager::sPrevTouch = false;

void InputManager::Init() {
    SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER);
    for (int i = 0; i < SDL_NumJoysticks(); ++i) {
        if (SDL_IsGameController(i)) {
            sController = SDL_GameControllerOpen(i);
            if (sController) break;
        }
    }
}

void InputManager::Shutdown() {
    if (sController) {
        SDL_GameControllerClose(sController);
        sController = nullptr;
    }
}

void InputManager::Update() {
    // Reset one-frame pulses
    sState.fireCannonPressed = false;
    sState.fireNukePressed = false;
    sState.fireMegalaserPressed = false;
    sState.pausePressed = false;
    sState.confirmPressed = false;
    sState.cancelPressed = false;
    sState.touchPressed = false;
    sState.touchReleased = false;

    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_CONTROLLERDEVICEADDED && !sController) {
            sController = SDL_GameControllerOpen(event.cdevice.which);
        } else if (event.type == SDL_CONTROLLERDEVICEREMOVED && sController) {
            SDL_GameControllerClose(sController);
            sController = nullptr;
        } else if (event.type == SDL_FINGERDOWN) {
            sState.touchDown = true;
            sState.touchX = event.tfinger.x * SCREEN_WIDTH;
            sState.touchY = event.tfinger.y * SCREEN_HEIGHT;
            sState.touchPressed = true;
        } else if (event.type == SDL_FINGERMOTION) {
            sState.touchX = event.tfinger.x * SCREEN_WIDTH;
            sState.touchY = event.tfinger.y * SCREEN_HEIGHT;
        } else if (event.type == SDL_FINGERUP) {
            sState.touchDown = false;
            sState.touchReleased = true;
        }
    }

    float moveX = 0.0f;
    float aimX = 0.0f;
    float aimY = 0.0f;
    bool btnCannon = false;
    bool btnNuke = false;
    bool btnLaser = false;
    bool btnPause = false;
    bool btnConfirm = false;
    bool btnCancel = false;

    if (sController) {
        // Left Stick Movement
        int16_t rawMoveX = SDL_GameControllerGetAxis(sController, SDL_CONTROLLER_AXIS_LEFTX);
        float normMoveX = (float)rawMoveX / 32767.0f;
        if (std::abs(normMoveX) > 0.2f) {
            moveX = normMoveX;
        }

        // D-Pad override
        if (SDL_GameControllerGetButton(sController, SDL_CONTROLLER_BUTTON_DPAD_LEFT)) {
            moveX = -1.0f;
        } else if (SDL_GameControllerGetButton(sController, SDL_CONTROLLER_BUTTON_DPAD_RIGHT)) {
            moveX = 1.0f;
        }

        // Right Stick Aiming
        int16_t rawAimX = SDL_GameControllerGetAxis(sController, SDL_CONTROLLER_AXIS_RIGHTX);
        int16_t rawAimY = SDL_GameControllerGetAxis(sController, SDL_CONTROLLER_AXIS_RIGHTY);
        float normAimX = (float)rawAimX / 32767.0f;
        float normAimY = (float)rawAimY / 32767.0f;

        float aimMagSq = normAimX * normAimX + normAimY * normAimY;
        if (aimMagSq > (0.25f * 0.25f)) {
            aimX = normAimX;
            aimY = normAimY;
            sState.hasAimInput = true;

            // Compute angle in degrees: 0 is right, -90 is up, -180 is left
            float rad = std::atan2(aimY, aimX);
            float deg = rad * 180.0f / 3.14159265f;

            // Clamp aiming angle so it remains above the ground plane
            if (deg > 0.0f && deg <= 90.0f) deg = 0.0f;       // clamp down-right to flat right
            if (deg > 90.0f && deg <= 180.0f) deg = -180.0f;  // clamp down-left to flat left

            sState.aimAngleDegrees = deg;

            // Twin-stick auto-fire threshold
            if (aimMagSq > (0.45f * 0.45f)) {
                btnCannon = true;
            }
        }

        // Action Buttons
        btnCannon |= SDL_GameControllerGetButton(sController, SDL_CONTROLLER_BUTTON_A);
        btnCannon |= SDL_GameControllerGetButton(sController, SDL_CONTROLLER_BUTTON_RIGHTSHOULDER);

        btnNuke = SDL_GameControllerGetButton(sController, SDL_CONTROLLER_BUTTON_Y) ||
                  SDL_GameControllerGetButton(sController, SDL_CONTROLLER_BUTTON_LEFTSHOULDER);

        btnLaser = SDL_GameControllerGetButton(sController, SDL_CONTROLLER_BUTTON_X);
        btnPause = SDL_GameControllerGetButton(sController, SDL_CONTROLLER_BUTTON_START);
        btnConfirm = SDL_GameControllerGetButton(sController, SDL_CONTROLLER_BUTTON_A);
        btnCancel = SDL_GameControllerGetButton(sController, SDL_CONTROLLER_BUTTON_B);
    }

    sState.moveAxisX = moveX;
    sState.aimAxisX = aimX;
    sState.aimAxisY = aimY;
    sState.fireCannon = btnCannon;

    // Edge triggers
    if (btnCannon && !sPrevFireCannon) sState.fireCannonPressed = true;
    if (btnNuke && !sPrevFireNuke) sState.fireNukePressed = true;
    if (btnLaser && !sPrevFireMegalaser) sState.fireMegalaserPressed = true;
    if (btnPause && !sPrevPause) sState.pausePressed = true;
    if (btnConfirm && !sPrevConfirm) sState.confirmPressed = true;
    if (btnCancel && !sPrevCancel) sState.cancelPressed = true;

    sPrevFireCannon = btnCannon;
    sPrevFireNuke = btnNuke;
    sPrevFireMegalaser = btnLaser;
    sPrevPause = btnPause;
    sPrevConfirm = btnConfirm;
    sPrevCancel = btnCancel;
}

} // namespace HeavyWeapon
