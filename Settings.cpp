#include "Settings.h"
#include "script.h" // For UI drawing functions like DrawMenuOption, DrawPairedMenuOption, DrawMenuHeader, ClampMenuIndex
#include "input.h"  // For IsKeyJustUp, PadPressed, and button definitions
#include <fstream>  // For file I/O
#include <string>   // For string manipulation
#include <vector>   // For std::vector
#include <map>      // For mapping virtual key codes to strings

// --- Global Settings Variables Definitions ---
bool g_autoLoadCharacter = false; // Changed default to false
bool g_showMoneyDisplay = true;
bool g_showRankBar = true;

// Keybinds
int g_keyboardMenuKey = VK_F4; // Default F4 (115)
int g_controllerMenuButton1 = BTN_RB; // Default RB
int g_controllerMenuButton2 = BTN_A;  // Default A

// --- File Constant ---
const char* settingsFile = "GTAOfflineSettings.ini";

// --- Helper to convert string to bool ---
bool stringToBool(const std::string& s) {
    return (s == "true" || s == "1");
}

// --- Helper to get string representation of a virtual key code ---
std::string GetKeyName(int keyCode) {
    if (keyCode >= 'A' && keyCode <= 'Z') return std::string(1, (char)keyCode);
    if (keyCode >= '0' && keyCode <= '9') return std::string(1, (char)keyCode);
    // Modified to only return F-key names for F1-F12.
    if (keyCode >= VK_F1 && keyCode <= VK_F12) return "F" + std::to_string(keyCode - VK_F1 + 1);
    switch (keyCode) {
    case VK_LBUTTON: return "LMB";
    case VK_RBUTTON: return "RMB";
    case VK_MBUTTON: return "MMB";
    case VK_BACK: return "BACKSPACE";
    case VK_TAB: return "TAB";
    case VK_RETURN: return "ENTER";
    case VK_SHIFT: return "SHIFT"; // VK_LSHIFT, VK_RSHIFT
    case VK_CONTROL: return "CTRL"; // VK_LCONTROL, VK_RCONTROL
    case VK_MENU: return "ALT"; // VK_LMENU, VK_RMENU
    case VK_PAUSE: return "PAUSE";
    case VK_CAPITAL: return "CAPS LOCK";
    case VK_ESCAPE: return "ESC";
    case VK_SPACE: return "SPACE";
    case VK_PRIOR: return "PAGE UP";
    case VK_NEXT: return "PAGE DOWN";
    case VK_END: return "END";
    case VK_HOME: return "HOME";
    case VK_LEFT: return "LEFT ARROW";
    case VK_UP: return "UP ARROW";
    case VK_RIGHT: return "RIGHT ARROW";
    case VK_DOWN: return "DOWN ARROW";
    case VK_INSERT: return "INSERT";
    case VK_DELETE: return "DELETE";
    case VK_NUMPAD0: return "NUMPAD 0";
    case VK_NUMPAD1: return "NUMPAD 1";
    case VK_NUMPAD2: return "NUMPAD 2";
    case VK_NUMPAD3: return "NUMPAD 3";
    case VK_NUMPAD4: return "NUMPAD 4";
    case VK_NUMPAD5: return "NUMPAD 5";
    case VK_NUMPAD6: return "NUMPAD 6";
    case VK_NUMPAD7: return "NUMPAD 7";
    case VK_NUMPAD8: return "NUMPAD 8";
    case VK_NUMPAD9: return "NUMPAD 9";
    case VK_MULTIPLY: return "NUMPAD *";
    case VK_ADD: return "NUMPAD +";
    case VK_SUBTRACT: return "NUMPAD -";
    case VK_DECIMAL: return "NUMPAD .";
    case VK_DIVIDE: return "NUMPAD /";
    case VK_NUMLOCK: return "NUM LOCK";
    case VK_SCROLL: return "SCROLL LOCK";
    case VK_OEM_1: return ";:";
    case VK_OEM_PLUS: return "+";
    case VK_OEM_COMMA: return ",";
    case VK_OEM_MINUS: return "-";
    case VK_OEM_PERIOD: return ".";
    case VK_OEM_2: return "/?";
    case VK_OEM_3: return "`~";
    case VK_OEM_4: return "[{";
    case VK_OEM_5: return "\\|";
    case VK_OEM_6: return "]}";
    case VK_OEM_7: return "'\"";
    default: return "Unknown (" + std::to_string(keyCode) + ")";
    }
}

// Helper function to check if a key is suitable for the menu toggle bind (ONLY F1-F12)
bool IsInvalidMenuKey(int key) {
    // Only F1 through F12 are valid for the keyboard menu key.
    if (key >= VK_F1 && key <= VK_F12) {
        return false; // This is a VALID F-key
    }
    // All other keys are invalid for this specific menu keybind.
    return true;
}


// --- Helper to get string representation of a controller button ---
std::string GetControllerButtonName(int buttonCode) {
    switch (buttonCode) {
    case BTN_A: return "A";
    case BTN_B: return "B";
    case BTN_X: return "X";
    case BTN_Y: return "Y";
    case BTN_LB: return "LB";
    case BTN_RB: return "RB";
    case BTN_BACK: return "BACK";
    case BTN_START: return "START";
    case BTN_LS: return "LS"; // Left Stick Click
    case BTN_RS: return "RS"; // Right Stick Click
    case DPAD_UP: return "D-PAD UP";
    case DPAD_DOWN: return "D-PAD DOWN";
    case DPAD_LEFT: return "D-PAD LEFT";
    case DPAD_RIGHT: return "D-PAD RIGHT";
    default: return "Btn:" + std::to_string(buttonCode);
    }
}


// --- Settings Initialization ---
void Settings_Init() {
    // Any one-time initialization for settings can go here.
}

// --- Function to save settings to an INI file ---
void SaveSettings() {
    std::ofstream file(settingsFile);
    if (file.is_open()) {
        file << "[Display]" << std::endl;
        file << "ShowMoneyDisplay=" << (g_showMoneyDisplay ? "true" : "false") << std::endl;
        file << "ShowRankBar=" << (g_showRankBar ? "true" : "false") << std::endl;
        file << "[Gameplay]" << std::endl;
        file << "AutoLoadCharacter=" << (g_autoLoadCharacter ? "true" : "false") << std::endl;
        file << "[Keybinds]" << std::endl;
        file << "KeyboardMenuKey=" << g_keyboardMenuKey << std::endl;
        file << "ControllerMenuButton1=" << g_controllerMenuButton1 << std::endl;
        file << "ControllerMenuButton2=" << g_controllerMenuButton2 << std::endl;
        file.close();
    }
}

// --- Function to load settings from an INI file ---
void LoadSettings() {
    std::ifstream file(settingsFile);
    if (file.is_open()) {
        std::string line;
        std::string currentSection;
        while (std::getline(file, line)) {
            line.erase(0, line.find_first_not_of(" \t\r\n"));
            line.erase(line.find_last_not_of(" \t\r\n") + 1);

            if (line.empty() || line[0] == ';') {
                continue;
            }
            if (line[0] == '[' && line.back() == ']') {
                currentSection = line.substr(1, line.length() - 2);
            }
            else {
                size_t equalsPos = line.find('=');
                if (equalsPos != std::string::npos) {
                    std::string key = line.substr(0, equalsPos);
                    std::string value = line.substr(equalsPos + 1);

                    key.erase(0, key.find_first_not_of(" \t\r\n"));
                    key.erase(key.find_last_not_of(" \t\r\n") + 1);
                    value.erase(0, value.find_first_not_of(" \t\r\n"));
                    value.erase(value.find_last_not_of(" \t\r\n") + 1);

                    if (currentSection == "Display") {
                        if (key == "ShowMoneyDisplay") g_showMoneyDisplay = stringToBool(value);
                        else if (key == "ShowRankBar") g_showRankBar = stringToBool(value);
                    }
                    else if (currentSection == "Gameplay") {
                        if (key == "AutoLoadCharacter") g_autoLoadCharacter = stringToBool(value);
                    }
                    else if (currentSection == "Keybinds") {
                        if (key == "KeyboardMenuKey") {
                            int loadedKey = std::stoi(value);
                            // Validate loaded key: if it's invalid (not an F-key), default to F4.
                            if (IsInvalidMenuKey(loadedKey)) { // IsInvalidMenuKey now checks for F-keys only
                                g_keyboardMenuKey = VK_F4; // Default to F4 (115) if an invalid key is loaded
                            }
                            else {
                                g_keyboardMenuKey = loadedKey;
                            }
                        }
                        else if (key == "ControllerMenuButton1") g_controllerMenuButton1 = std::stoi(value);
                        else if (key == "ControllerMenuButton2") g_controllerMenuButton2 = std::stoi(value);
                    }
                }
            }
        }
        file.close();
    }
}

// --- Settings Menu Function ---
int settingsMenuIndex = 0;
static bool listeningForKeybind = false;
static bool listeningForControllerBind = false;
static DWORD keybindListenStartTime = 0;
// Declared as static at file scope to fix E0020 error
static int capturedBtnCount = 0;
static int tempBtn1 = 0;


void draw_settings_menu() {
    extern const float MENU_X, MENU_Y, MENU_W, MENU_H;
    extern int inputDelayFrames, menuCategory, menuIndex;
    extern const RGBA BG_COLOR, HEADER_COLOR, OPTION_COLOR, SELECTED_COLOR, TEXT_COLOR, SELECTED_TEXT_COLOR, HEADER_TEXT_COLOR;

    const int numOptions = 5;
    const char* labels[numOptions] = {
        "Autoload Character", "Toggle Money Display", "Toggle Rank Bar",
        "Change Keyboard Keybind", "Change Controller Keybind"
    };

    // --- Drawing Logic ---
    float x = MENU_X, y = MENU_Y, w = MENU_W, h = MENU_H;
    float headerHeight = h;
    float optionsTotalHeight = h * (numOptions + 1);
    float totalMenuHeight = headerHeight + optionsTotalHeight;
    float menuBlockCenterY = y + (totalMenuHeight / 2.0f);
    GRAPHICS::DRAW_RECT(x + w * 0.5f, menuBlockCenterY, w, totalMenuHeight, BG_COLOR.r, BG_COLOR.g, BG_COLOR.b, BG_COLOR.a);
    float headerBgCenterY = y + (headerHeight * 0.5f);
    GRAPHICS::DRAW_RECT(x + w * 0.5f, headerBgCenterY, w, headerHeight, HEADER_COLOR.r, HEADER_COLOR.g, HEADER_COLOR.b, HEADER_COLOR.a);
    DrawMenuHeader("SETTINGS", x, y, w);

    float optionsStartY = y + headerHeight;
    for (int i = 0; i < numOptions; ++i) {
        char valueText[64];
        switch (i) {
        case 0: sprintf_s(valueText, "%s", g_autoLoadCharacter ? "ON" : "OFF"); break;
        case 1: sprintf_s(valueText, "%s", g_showMoneyDisplay ? "ON" : "OFF"); break;
        case 2: sprintf_s(valueText, "%s", g_showRankBar ? "ON" : "OFF"); break;
        case 3: sprintf_s(valueText, "%s", GetKeyName(g_keyboardMenuKey).c_str()); break;
        case 4: sprintf_s(valueText, "%s + %s", GetControllerButtonName(g_controllerMenuButton1).c_str(), GetControllerButtonName(g_controllerMenuButton2).c_str()); break;
        }
        DrawPairedMenuOption(labels[i], valueText, x, optionsStartY + h * i, w, h, i == settingsMenuIndex);
    }
    DrawMenuOption("Back", x, optionsStartY + h * numOptions, w, h, numOptions == settingsMenuIndex);
    ClampMenuIndex(settingsMenuIndex, numOptions + 1);

    // --- Input Handling for Keyboard Keybind Listening Mode ---
    if (listeningForKeybind) {
        UI::_SET_NOTIFICATION_TEXT_ENTRY("STRING");
        UI::_ADD_TEXT_COMPONENT_STRING("~y~Press a NEW F-key (F1-F12) for the menu bind. Press ESC to cancel.");
        UI::_DRAW_NOTIFICATION(false, true);

        // Timeout for keybind selection
        if (GetTickCount64() - keybindListenStartTime > 5000) {
            UI::_SET_NOTIFICATION_TEXT_ENTRY("STRING");
            UI::_ADD_TEXT_COMPONENT_STRING("~r~Keyboard keybind change timed out. Keybind not changed.");
            UI::_DRAW_NOTIFICATION(false, true);
            listeningForKeybind = false;
            inputDelayFrames = 30; // Apply delay to prevent immediate re-triggering of menu actions
            return;
        }

        // CRITICAL FIX: Only process new key presses AFTER the input delay has fully expired.
        // This ensures the key used to SELECT the option (e.g., Numpad 5/Enter) is not captured.
        // Calling IsKeyJustUp again here will clear its "just up" state.
        if (inputDelayFrames > 0) {
            return; // Still in delay period, ignore all key presses
        }

        // Iterate through all possible keys to find the one that was just released.
        for (int key = 1; key < 255; ++key) {
            if (IsKeyJustUp(key)) { // Only react to keys that were just released
                if (key == VK_ESCAPE) {
                    UI::_SET_NOTIFICATION_TEXT_ENTRY("STRING");
                    UI::_ADD_TEXT_COMPONENT_STRING("~y~Keyboard keybind change cancelled.");
                    UI::_DRAW_NOTIFICATION(false, true);
                    listeningForKeybind = false;
                    inputDelayFrames = 30; // Apply delay after cancellation
                    return;
                }

                // Validate the pressed key: now only F1-F12 are valid.
                if (IsInvalidMenuKey(key)) { // IsInvalidMenuKey now specifically checks for F-keys
                    UI::_SET_NOTIFICATION_TEXT_ENTRY("STRING");
                    std::string msg = "~r~Invalid key: " + GetKeyName(key) + ". Only F1-F12 allowed. Keybind NOT changed.";
                    char msg_buffer[128];
                    strncpy_s(msg_buffer, msg.c_str(), sizeof(msg_buffer));
                    msg_buffer[sizeof(msg_buffer) - 1] = 0;
                    UI::_ADD_TEXT_COMPONENT_STRING(msg_buffer);
                    UI::_DRAW_NOTIFICATION(false, true);
                    // IMPORTANT: g_keyboardMenuKey retains its previous (valid) value.
                    // IMPORTANT: SaveSettings() is NOT called here.
                    listeningForKeybind = false; // Stop listening
                    inputDelayFrames = 30; // Apply delay after notification
                    return;
                }

                // If we reach here, the key is a valid F-key and can be set as the new bind.
                g_keyboardMenuKey = key;
                SaveSettings(); // Save the new, valid key to the INI file
                UI::_SET_NOTIFICATION_TEXT_ENTRY("STRING");
                UI::_ADD_TEXT_COMPONENT_STRING("~g~Keyboard menu key updated!");
                UI::_DRAW_NOTIFICATION(false, true);
                listeningForKeybind = false;
                inputDelayFrames = 30; // Apply delay after successful update
                return;
            }
        }
    }
    // --- Input Handling for Controller Keybind Listening Mode ---
    else if (listeningForControllerBind) {
        UI::_SET_NOTIFICATION_TEXT_ENTRY("STRING");
        UI::_ADD_TEXT_COMPONENT_STRING("~y~Press two controller buttons for the menu bind. Press B to cancel.");
        UI::_DRAW_NOTIFICATION(false, true);

        if (GetTickCount64() - keybindListenStartTime > 7000) {
            UI::_SET_NOTIFICATION_TEXT_ENTRY("STRING");
            UI::_ADD_TEXT_COMPONENT_STRING("~r~Controller keybind change timed out.");
            UI::_DRAW_NOTIFICATION(false, true);
            listeningForControllerBind = false;
            capturedBtnCount = 0;
            inputDelayFrames = 30;
            return;
        }

        // Apply input delay check for controller as well.
        if (inputDelayFrames > 0) {
            return;
        }

        // capturedBtnCount and tempBtn1 are now static variables at file scope.

        if (PadPressed(BTN_B)) { // Using PadPressed as per original code for cancellation
            UI::_SET_NOTIFICATION_TEXT_ENTRY("STRING");
            UI::_ADD_TEXT_COMPONENT_STRING("~y~Controller keybind change cancelled.");
            UI::_DRAW_NOTIFICATION(false, true);
            listeningForControllerBind = false;
            capturedBtnCount = 0;
            inputDelayFrames = 30;
            return;
        }

        int currentPressedBtn = 0;
        // Reverted to PadHeld as IsPadJustPressed was undefined and caused compilation errors.
        // This part of the code is restored to its previous state before the error was introduced.
        if (PadHeld(BTN_A)) currentPressedBtn = BTN_A; else if (PadHeld(BTN_B)) currentPressedBtn = BTN_B;
        else if (PadHeld(BTN_X)) currentPressedBtn = BTN_X; else if (PadHeld(BTN_Y)) currentPressedBtn = BTN_Y;
        else if (PadHeld(BTN_LB)) currentPressedBtn = BTN_LB; else if (PadHeld(BTN_RB)) currentPressedBtn = BTN_RB;
        else if (PadHeld(BTN_BACK)) currentPressedBtn = BTN_BACK; else if (PadHeld(BTN_START)) currentPressedBtn = BTN_START;
        else if (PadHeld(BTN_LS)) currentPressedBtn = BTN_LS; else if (PadHeld(BTN_RS)) currentPressedBtn = BTN_RS;
        else if (PadHeld(DPAD_UP)) currentPressedBtn = DPAD_UP; else if (PadHeld(DPAD_DOWN)) currentPressedBtn = DPAD_DOWN;
        else if (PadHeld(DPAD_LEFT)) currentPressedBtn = DPAD_LEFT; else if (PadHeld(DPAD_RIGHT)) currentPressedBtn = DPAD_RIGHT;

        if (currentPressedBtn != 0) {
            if (capturedBtnCount == 0) {
                tempBtn1 = currentPressedBtn;
                capturedBtnCount = 1;
                UI::_SET_NOTIFICATION_TEXT_ENTRY("STRING");
                UI::_ADD_TEXT_COMPONENT_STRING("~y~First button captured. Press second button.");
                UI::_DRAW_NOTIFICATION(false, true);
                inputDelayFrames = 15; // Delay to prevent double capture or immediate re-read
            }
            else if (capturedBtnCount == 1 && currentPressedBtn != tempBtn1) {
                g_controllerMenuButton1 = tempBtn1;
                g_controllerMenuButton2 = currentPressedBtn;
                SaveSettings();
                UI::_SET_NOTIFICATION_TEXT_ENTRY("STRING");
                UI::_ADD_TEXT_COMPONENT_STRING("~g~Controller menu key updated!");
                UI::_DRAW_NOTIFICATION(false, true);
                listeningForControllerBind = false;
                capturedBtnCount = 0;
                inputDelayFrames = 30;
            }
        }
    }
    // --- Normal Menu Navigation Input Handling ---
    else {
        // If there's an active input delay, prevent any menu navigation.
        if (inputDelayFrames > 0) {
            return;
        }

        // Handle menu up/down navigation
        if (IsKeyJustUp(VK_NUMPAD8) || PadPressed(DPAD_UP)) {
            settingsMenuIndex = (settingsMenuIndex - 1 + (numOptions + 1)) % (numOptions + 1);
            inputDelayFrames = 10;
        }
        else if (IsKeyJustUp(VK_NUMPAD2) || PadPressed(DPAD_DOWN)) {
            settingsMenuIndex = (settingsMenuIndex + 1) % (numOptions + 1);
            inputDelayFrames = 10;
        }

        static bool prevA = false;
        bool currA = PadPressed(BTN_A);
        bool keyboardSelect = IsKeyJustUp(VK_NUMPAD5) || IsKeyJustUp(VK_RETURN);
        bool controllerSelect = (currA && !prevA);

        if (keyboardSelect || controllerSelect) {
            switch (settingsMenuIndex) {
            case 0: g_autoLoadCharacter = !g_autoLoadCharacter; SaveSettings(); inputDelayFrames = 10; break;
            case 1: g_showMoneyDisplay = !g_showMoneyDisplay; SaveSettings(); inputDelayFrames = 10; break;
            case 2: g_showRankBar = !g_showRankBar; SaveSettings(); inputDelayFrames = 10; break;
            case 3: // Change Keyboard Keybind
                if (keyboardSelect) {
                    listeningForKeybind = true;
                    keybindListenStartTime = GetTickCount64();
                    // Set a significant delay. This is crucial for separating selection from new key input.
                    inputDelayFrames = 30;
                    // CRITICAL FIX: Explicitly consume the key that just activated this option.
                    // This ensures it is NOT read as the new keybind in the very next frame.
                    // Calling IsKeyJustUp again here will clear its "just up" state.
                    IsKeyJustUp(VK_NUMPAD5);
                    IsKeyJustUp(VK_RETURN);
                }
                else {
                    UI::_SET_NOTIFICATION_TEXT_ENTRY("STRING");
                    UI::_ADD_TEXT_COMPONENT_STRING("~r~Please use the keyboard (Enter/Numpad5) to change this setting.");
                    UI::_DRAW_NOTIFICATION(false, true);
                    inputDelayFrames = 30;
                }
                break;
            case 4: // Change Controller Keybind
                if (controllerSelect) {
                    listeningForControllerBind = true;
                    keybindListenStartTime = GetTickCount64();
                    inputDelayFrames = 30; // Set a significant delay here as well.
                    // Consume the button that just activated this option.
                    PadPressed(BTN_A); // Clear its state for the next frame
                }
                else {
                    UI::_SET_NOTIFICATION_TEXT_ENTRY("STRING");
                    UI::_ADD_TEXT_COMPONENT_STRING("~r~Please use the controller (A Button) to change this setting.");
                    UI::_DRAW_NOTIFICATION(false, true);
                    inputDelayFrames = 30;
                }
                break;
            case 5: // Back
                menuCategory = 0;
                menuIndex = 8;
                inputDelayFrames = 10;
                break;
            }
        }
        prevA = currA;
    }
}
