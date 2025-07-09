#pragma once
#include <windows.h>
#include <Xinput.h>
#pragma comment(lib, "Xinput9_1_0.lib")

extern XINPUT_STATE g_padPrev;
extern XINPUT_STATE g_padCurr;

void PollPad();
bool PadHeld(int btn); // Defined in input.cpp

#define BTN_A        0x1000
#define BTN_B        0x2000
#define BTN_X        0x4000
#define BTN_Y        0x8000
#define BTN_LB       0x0100
#define BTN_RB       0x0200
#define BTN_LS       0x0587
#define BTN_RS       0x0588
#define DPAD_UP      0x0001
#define DPAD_DOWN    0x0002
#define DPAD_LEFT    0x0004
#define DPAD_RIGHT   0x0008
#define LEFT_X 0
#define LEFT_Y 1
#define IsKeyDown(key) ((GetAsyncKeyState(key) & 0x8000) != 0)
// --- GTA V Native Input Codes ---
#define INPUT_ATTACK        24   // Left Mouse / RT (shoot/punch)
#define INPUT_MELEE_ATTACK  140  // Melee Attack (fist/punch)
#define INPUT_AIM           25   // Right Mouse / LT
#define INPUT_JUMP          22   // Spacebar / X
#define INPUT_SPRINT        21   // Shift / A

inline bool RT_Held() {
    extern XINPUT_STATE g_padCurr;
    return g_padCurr.Gamepad.bRightTrigger > 32;
}
// NEW: Helper to check if the Left Trigger on a controller is held
inline bool LT_Held() {
    extern XINPUT_STATE g_padCurr;
    return g_padCurr.Gamepad.bLeftTrigger > 30; // 30 is a common threshold
}
inline float GetPadAxisLX() {
    extern XINPUT_STATE g_padCurr;
    SHORT raw = g_padCurr.Gamepad.sThumbLX;
    if (abs(raw) < XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE) return 0.0f;
    return (float)raw / 32767.0f;
}
inline float GetPadAxisLY() {
    extern XINPUT_STATE g_padCurr;
    SHORT raw = g_padCurr.Gamepad.sThumbLY;
    if (abs(raw) < XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE) return 0.0f;
    return (float)raw / 32767.0f;
}
inline float GetPadAxis(int axis) {
    if (axis == LEFT_X) return GetPadAxisLX();
    if (axis == LEFT_Y) return GetPadAxisLY();
    return 0.0f;
}
inline float GetPadAxisRY() {
    extern XINPUT_STATE g_padCurr;
    SHORT raw = g_padCurr.Gamepad.sThumbRY;
    if (abs(raw) < XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE) return 0.0f;
    return (float)raw / 32767.0f;
}
inline bool KeyPressed(int vk) { return (GetAsyncKeyState(vk) & 1); }
inline bool KeyHeld(int vk) { return (GetAsyncKeyState(vk) & 0x8000); }

// --- DPAD/Slider Repeat Logic (for all menus and sliders)
struct InputRepeatState {
    int delay = 0;
    int repeat = 0;
    bool wasHeld = false;
};

// Keyboard repeat for menu navigation/sliders
inline bool IsKeyJustUp(int vkey) {
    static InputRepeatState state[16];
    int idx = 0;
    switch (vkey) {
    case VK_NUMPAD4: idx = 0; break;
    case VK_NUMPAD6: idx = 1; break;
    case VK_NUMPAD8: idx = 2; break;
    case VK_NUMPAD2: idx = 3; break;
    case VK_F4:      idx = 4; break;
    case VK_NUMPAD5: idx = 5; break;
    case VK_NUMPAD7: idx = 6; break;
    case VK_NUMPAD9: idx = 7; break;
    case VK_NUMPAD0: idx = 8; break;
    default:         idx = 15; break;
    }
    bool held = KeyHeld(vkey);
    if (held) {
        if (!state[idx].wasHeld) {
            state[idx].wasHeld = true;
            state[idx].delay = 18;
            state[idx].repeat = 3;
            return true;
        }
        if (state[idx].delay > 0) {
            state[idx].delay--;
        }
        else if (state[idx].repeat > 0) {
            state[idx].repeat--;
        }
        else {
            state[idx].repeat = 3;
            return true;
        }
    }
    else {
        state[idx].wasHeld = false;
        state[idx].delay = 0;
        state[idx].repeat = 0;
    }
    return false;
}

// Controller DPAD/BTN repeat for menu navigation/sliders
inline bool PadPressed(int btn) {
    static InputRepeatState state[16];
    int idx = 0;
    switch (btn) {
    case DPAD_LEFT:  idx = 0; break;
    case DPAD_RIGHT: idx = 1; break;
    case DPAD_UP:    idx = 2; break;
    case DPAD_DOWN:  idx = 3; break;
    case BTN_A:      idx = 4; break;
    case BTN_B:      idx = 5; break;
    case BTN_LB:     idx = 6; break;
    case BTN_RB:     idx = 7; break;
    default:         idx = 15; break;
    }
    bool held = PadHeld(btn);
    if (held) {
        if (!state[idx].wasHeld) {
            state[idx].wasHeld = true;
            state[idx].delay = 18;
            state[idx].repeat = 3;
            return true;
        }
        if (state[idx].delay > 0) {
            state[idx].delay--;
        }
        else if (state[idx].repeat > 0) {
            state[idx].repeat--;
        }
        else {
            state[idx].repeat = 3;
            return true;
        }
    }
    else {
        state[idx].wasHeld = false;
        state[idx].delay = 0;
        state[idx].repeat = 0;
    }
    return false;
}
