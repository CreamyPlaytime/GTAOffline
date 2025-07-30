#include <Windows.h> // For ShellExecute
#include "input.h"
#include "script.h"

void open_link(const char* url) {
    ShellExecuteA(NULL, "open", url, NULL, NULL, SW_SHOWNORMAL);
}

void draw_credits_menu() {
    extern int menuCategory;
    extern int menuIndex;
    extern int inputDelayFrames;

    const float MENU_X = 0.02f;
    const float MENU_Y = 0.13f;
    const float MENU_W = 0.29f;
    const float MENU_H = 0.038f;

    struct CreditEntry {
        const char* label;
        const char* url; // Null means not clickable
    };

    CreditEntry creditLines[] = {
        { "Mod Created By: CreamyPlaytime", "https://www.youtube.com/@CreamyPlaytime" },
        { "ScriptHookV: Alexander Blade", "https://www.dev-c.com/gtav/scripthookv/" },
        { "Assistance: ChatGPT (OpenAI)", "https://chat.openai.com/" },
        { "Assistance: Google Gemini", "https://gemini.google.com/" },
        { "Game: Rockstar Games", "https://www.rockstargames.com/" },
        { "Special Thanks: You!", nullptr }
    };

    const int numCreditLines = sizeof(creditLines) / sizeof(creditLines[0]);
    const int numOptions = numCreditLines + 1; // +1 for Back
    float totalHeight = MENU_H * numOptions;

    GRAPHICS::DRAW_RECT(MENU_X + MENU_W * 0.5f, MENU_Y - MENU_H * 0.5f + totalHeight * 0.5f, MENU_W, totalHeight,
        BG_COLOR.r, BG_COLOR.g, BG_COLOR.b, BG_COLOR.a);

    DrawMenuHeader("Credits", MENU_X, MENU_Y, MENU_W);

    float optionY = MENU_Y + MENU_H;

    for (int i = 0; i < numCreditLines; ++i) {
        float currentY = optionY + (MENU_H * i);
        bool selected = (menuIndex == i);

        DrawMenuOption(creditLines[i].label, MENU_X, currentY, MENU_W, MENU_H, selected);
    }

    float backButtonY = optionY + (MENU_H * numCreditLines);
    bool backSelected = (menuIndex == numCreditLines);
    DrawMenuOption("Back", MENU_X, backButtonY, MENU_W, MENU_H, backSelected);

    // --- Navigation ---
    if (inputDelayFrames > 0) return;

    int maxIndex = numOptions - 1;
    if (IsKeyJustUp(VK_UP) || PadPressed(DPAD_UP)) {
        menuIndex = (menuIndex - 1 + numOptions) % numOptions;
        inputDelayFrames = 10;
    }
    if (IsKeyJustUp(VK_DOWN) || PadPressed(DPAD_DOWN)) {
        menuIndex = (menuIndex + 1) % numOptions;
        inputDelayFrames = 10;
    }

    if (IsKeyJustUp(VK_NUMPAD5) || IsKeyJustUp(VK_RETURN) || PadPressed(BTN_A)) {
        if (menuIndex == numCreditLines) {
            menuCategory = 0;
            menuIndex = 7;
            inputDelayFrames = 10;
        }
        else if (creditLines[menuIndex].url) {
            open_link(creditLines[menuIndex].url);
            inputDelayFrames = 15;
        }
    }

    if (IsKeyJustUp(VK_BACK) || IsKeyJustUp(VK_ESCAPE) || PadPressed(BTN_B)) {
      
        menuIndex = 7;
        inputDelayFrames = 10;
    }
}
void Credits_Init() {
    // Currently no setup needed, placeholder to resolve linker error
}