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
#define BTN_BACK     0x0020 // Added BTN_BACK
#define BTN_START    0x0010 // Added BTN_START
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

// --- Global Settings Variables Declarations (Definitions are in Settings.cpp) ---
extern bool g_autoLoadCharacter;
extern bool g_showMoneyDisplay;
extern bool g_showRankBar;

// Keybinds
extern int g_keyboardMenuKey;
extern int g_controllerMenuButton1;
extern int g_controllerMenuMenuButton2;
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
    // IMPORTANT: Increased array size to accommodate more distinct keys for repeat logic.
    // If you encounter "vector errors" related to `state` in input.h, it might be due to your compiler
    // not supporting C++11 or later, or not compiling with C++11/14/17 flags.
    static InputRepeatState state[30]; // Changed from 16 to 30
    int idx = 0;
    // Assign unique indices for common menu keys, including arrow keys and Enter/Esc
    if (vkey == VK_NUMPAD4) idx = 0;
    else if (vkey == VK_NUMPAD6) idx = 1;
    else if (vkey == VK_NUMPAD8) idx = 2;
    else if (vkey == VK_NUMPAD2) idx = 3;
    else if (vkey == VK_F4)      idx = 4;
    else if (vkey == VK_NUMPAD5) idx = 5;
    else if (vkey == VK_NUMPAD7) idx = 6;
    else if (vkey == VK_NUMPAD9) idx = 7;
    else if (vkey == VK_NUMPAD0) idx = 8;
    else if (vkey == VK_UP)      idx = 9;   // Up Arrow
    else if (vkey == VK_DOWN)    idx = 10;  // Down Arrow
    else if (vkey == VK_LEFT)    idx = 11;  // Left Arrow
    else if (vkey == VK_RIGHT)   idx = 12;  // Right Arrow
    else if (vkey == VK_RETURN)  idx = 13;  // Enter Key
    else if (vkey == VK_ESCAPE)  idx = 14;  // Escape Key
    else idx = 29; // Default for other keys

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
    case BTN_BACK:   idx = 8; break; // Added index for BTN_BACK
    case BTN_START:  idx = 9; break; // Added index for BTN_START
    case BTN_LS:     idx = 10; break;
    case BTN_RS:     idx = 11; break;
    case BTN_X:      idx = 12; break;
    case BTN_Y:      idx = 13; break;
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
