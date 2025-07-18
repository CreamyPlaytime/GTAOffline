#include "Credits.h"
#include "input.h"
#include "script.h" // For menu category enum

void Credits_Init() {
    // Nothing to initialize for now
}

void draw_credits_menu() {
    extern int menuCategory;
    extern int menuIndex;
    extern int inputDelayFrames;

    const float MENU_X = 0.02f;
    const float MENU_Y = 0.13f;
    const float MENU_W = 0.29f;
    const float MENU_H = 0.038f;

    const char* creditLines[] = {
        "Mod Created By: CreamyPlaytime",
        "Special Thanks: You!",
        "ScriptHookV: Alexander Blade",
        "Assistance: OpenAI & Google Gemini",
        "Game: Rockstar Games"
    };
    const int numCreditLines = sizeof(creditLines) / sizeof(creditLines[0]);
    // Total options = header(1) + credit lines + back button(1)
    const int numOptions = 1 + numCreditLines + 1;
    float totalHeight = MENU_H * numOptions;

    // --- Draw Background and Header ---
    GRAPHICS::DRAW_RECT(MENU_X + MENU_W * 0.5f, MENU_Y - MENU_H * 0.5f + totalHeight * 0.5f, MENU_W, totalHeight, BG_COLOR.r, BG_COLOR.g, BG_COLOR.b, BG_COLOR.a);
    DrawMenuHeader("Credits", MENU_X, MENU_Y, MENU_W);

    // --- Draw Options ---
    float optionY = MENU_Y + MENU_H;

    // 1. Draw the non-interactive credit lines
    for (int i = 0; i < numCreditLines; ++i) {
        float currentY = optionY + (MENU_H * i);
        // Draw the background rect for the text
        GRAPHICS::DRAW_RECT(MENU_X + MENU_W * 0.5f, currentY + (MENU_H - 0.004f) * 0.5f, MENU_W, MENU_H - 0.004f,
            OPTION_COLOR.r, OPTION_COLOR.g, OPTION_COLOR.b, OPTION_COLOR.a);

        // Draw the text itself
        UI::SET_TEXT_FONT(FONT);
        UI::SET_TEXT_SCALE(0.0f, 0.36f);
        UI::SET_TEXT_COLOUR(TEXT_COLOR.r, TEXT_COLOR.g, TEXT_COLOR.b, TEXT_COLOR.a);
        UI::_SET_TEXT_ENTRY("STRING");
        UI::_ADD_TEXT_COMPONENT_STRING((char*)creditLines[i]);
        UI::_DRAW_TEXT(MENU_X + 0.017f, currentY + 0.007f);
    }

    // 2. Draw the interactive "Back" button
    float backButtonY = optionY + (MENU_H * numCreditLines);
    // Since it's the only option, it's always selected
    DrawMenuOption("Back", MENU_X, backButtonY, MENU_W, MENU_H, true);

    // --- Navigation ---
    if (inputDelayFrames > 0) return;

    // There is only one selectable option, so no up/down navigation is needed.
    menuIndex = 0; // Or whatever index corresponds to the back button

    // Corrected: Added VK_RETURN, VK_BACK, VK_ESCAPE for going back
    if (IsKeyJustUp(VK_NUMPAD5) || IsKeyJustUp(VK_RETURN) || IsKeyJustUp(VK_BACK) || IsKeyJustUp(VK_ESCAPE) || PadPressed(BTN_A) || PadPressed(BTN_B)) {
        menuCategory = 0; // Go back to CAT_MAIN
        menuIndex = 7;    // Highlight the "Credits" option on the main menu
        inputDelayFrames = 10;
    }
}