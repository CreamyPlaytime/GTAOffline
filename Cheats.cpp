#define _CRT_SECURE_NO_WARNINGS
#define NOMINMAX
#include "Cheats.h"
#include "script.h"
#include "Self.h"
#include "Weapons.h"
#include "Misc.h"
#include "input.h"

// This variable keeps track of which tab is currently open.
int cheatTab = 0;

// This function is called once to initialize the state of the cheat menu.
void Cheats_Init() {
    cheatTab = 0;
    Self_Init();
    Weapons_Init();
    Misc_Init();
}

// This function is called on every game tick to update the logic for the cheats.
void Cheats_Tick() {
    Self_Tick();
    Weapons_Tick();
    Misc_Tick();
}

// This function handles drawing the entire cheat menu, including the header, tabs, and the content of the selected tab.
void Cheats_DrawMenu(int& menuIndex, float x, float y, float w, float h) {
    extern int inputDelayFrames; // Declare extern to use global inputDelayFrames

    // --- Determine the number of options for the current tab ---
    int numOptions = 0;
    switch (cheatTab) {
    case 0: numOptions = Self_GetNumOptions(); break;
    case 1: numOptions = Weapons_GetNumOptions(); break;
    case 2: numOptions = Misc_GetNumOptions(); break;
    }

    // --- Background Calculation (Corrected) ---
    // The total number of rows includes the tab bar (1) and all options.
    int totalRows = 1 + numOptions;
    float totalHeight = h * totalRows;
    // The center of the background is calculated from the top of the tab bar.
    float bg_center_y = y + (totalHeight / 2.0f) - (h / 2.0f);

    // Draw ONE unified background for the tabs and options.
    GRAPHICS::DRAW_RECT(x + w * 0.5f, bg_center_y, w, totalHeight, BG_COLOR.r, BG_COLOR.g, BG_COLOR.b, BG_COLOR.a);

    // --- Element Drawing (Corrected Positions) ---

    // 1. Draw the Tab Bar in the top-most slot.
    // This now serves as the menu header.
    const char* tabNames[3] = { "Self", "Weapons", "Misc" };
    float tab_bar_y = y; // The first row of the content area.
    float tabSectionWidth = w / 3.0f;

    for (int i = 0; i < 3; ++i) {
        float tabX = x + (tabSectionWidth * i) + (tabSectionWidth / 2.0f);
        RGBA currentTabColor = (i == cheatTab) ? SELECTED_TAB_COLOR : HEADER_COLOR; // Use header color for inactive tabs

        // Draw the tab background, filling the entire width of its section.
        GRAPHICS::DRAW_RECT(tabX, tab_bar_y + (h * 0.5f), tabSectionWidth, h, currentTabColor.r, currentTabColor.g, currentTabColor.b, currentTabColor.a);

        // Draw the tab text, centered within the tab background.
        UI::SET_TEXT_FONT(FONT);
        UI::SET_TEXT_SCALE(0.0f, 0.40f); // Slightly larger font for header tabs
        UI::SET_TEXT_COLOUR(TEXT_COLOR.r, TEXT_COLOR.g, TEXT_COLOR.b, TEXT_COLOR.a);
        UI::SET_TEXT_CENTRE(1);
        UI::_SET_TEXT_ENTRY("STRING");
        UI::_ADD_TEXT_COMPONENT_STRING((char*)tabNames[i]);
        UI::_DRAW_TEXT(tabX, tab_bar_y + 0.005f);
        UI::SET_TEXT_CENTRE(0); // Reset alignment
    }

    // --- Handle Tab Switching Input ---
    // Only process input if no global input delay
    if (inputDelayFrames == 0) {
        // Corrected: Use VK_NUMPAD7 and VK_OEM_4 for left, VK_NUMPAD9 and VK_OEM_6 for right
        if (IsKeyJustUp(VK_NUMPAD7) || IsKeyJustUp(VK_OEM_4) || PadPressed(BTN_LB)) {
            cheatTab = (cheatTab + 2) % 3; // Go left
            menuIndex = 0;
            inputDelayFrames = 15; // Increased delay from 10 to 15
        }
        if (IsKeyJustUp(VK_NUMPAD9) || IsKeyJustUp(VK_OEM_6) || PadPressed(BTN_RB)) {
            cheatTab = (cheatTab + 1) % 3; // Go right
            menuIndex = 0;
            inputDelayFrames = 15; // Increased delay from 10 to 15
        }
    }

    // 2. Draw the Options List, starting one row below the tab bar.
    float options_y_start = y + h;
    switch (cheatTab) {
    case 0: Self_DrawMenu(menuIndex, x, options_y_start, w, h); break;
    case 1: Weapons_DrawMenu(menuIndex, x, options_y_start, w, h); break;
    case 2: Misc_DrawMenu(menuIndex, x, options_y_start, w, h); break;
    }
}