#define _CRT_SECURE_NO_WARNINGS
#include "script.h"
#include "Vehicle.h"
#include "RankBar.h"
#include "CharacterCreator.h"
#include "RpEvents.h"
#include "Money.h"
#include "Cheats.h"
#include "input.h"
#include "CarShop.h"
#include "Garage.h"
#include "GunStore.h" // Include GunStore.h for the new function
#include "Credits.h"
#include "CarExport.h"
#include "Properties.h" // Ensure Properties.h is included
#include "Settings.h"   // Include Settings.h for settings variables and functions
#include <windows.h>
#include <ctime>
#include <cstdio>
#include <vector>
#include <algorithm>
#include <string> // For std::string

// Defensive macros
#define VALID_PED(ped)     ((ped) != 0 && ENTITY::DOES_ENTITY_EXIST(ped))
#define VALID_PLAYER(p)   ((p) != -1 && PLAYER::IS_PLAYER_PLAYING(p))

#pragma warning(disable : 4244 4305)

// --- File Constants ---
const char* characterFile = "GTAOfflineChar.ini";
const char* playerStatsFile = "GTAOfflinePlayerStats.ini";
const char* xpFile = "GTAOfflineXP.ini";
const char* propertiesFile = "GTAOfflineProperties.ini"; // Define properties file name
// const char* gunLockerFile = "GTAOfflineGunLocker.ini"; // GunStore.cpp uses hardcoded path, so this isn't directly used here for GunStore_Save/Load

// Protagonist Model Hashes (defined locally in .cpp)
const Hash FRANKLIN_MODEL_HASH = 0xB881550E; // "player_a"
const Hash MICHAEL_MODEL_HASH = 0xD71149EE; // "player_b"
const Hash TREVOR_MODEL_HASH = 0x98FF33E7; // "player_c"

// Consolidated Protagonist specific save files (defined locally in .cpp)
const char* PROTAGONIST_MONEY_FILE = "GTAOfflineMoney_Protagonist.ini";
const char* PROTAGONIST_RANK_FILE = "GTAOfflineRank_Protagonist.ini";
const char* PROTAGONIST_XP_FILE = "GTAOfflineXP_Protagonist.ini";
const char* PROTAGONIST_PROPERTIES_FILE = "GTAOfflineProperties_Protagonist.ini";
// const char* PROTAGONIST_GUNLOCKER_FILE = "GTAOfflineGunLocker_Protagonist.ini"; // Not used as GunStore.cpp is hardcoded


// --- Game Logic Constants ---
const int BRIBE_AMOUNT = 5000; // Define the bribe amount here

// --- Menu State ---
// Definition of the Category enum
enum Category {
    CAT_MAIN = 0, CAT_CHARACTER, CAT_CHEATS, CAT_VEHICLE,
    CAT_SAVELOAD, CAT_CAR_SHOP, CAT_GARAGE, CAT_GUN_STORE, CAT_CREDITS, CAT_SETTINGS // Added CAT_SETTINGS
};
int menuCategory = CAT_MAIN;
int menuIndex = 0;
bool menuOpen = false;
int inputDelayFrames = 0; // Global input delay for menu navigation/selection

// --- UI Constants ---
const float MENU_X = 0.02f;
const float MENU_Y = 0.13f; // This is now the Y coordinate for the TOP of the menu block
const float MENU_W = 0.29f;
const float MENU_H = 0.038f; // Height of a single option/header row

// --- UI Color Palette Definition (These are now const and defined here) ---
const int FONT = 0;
const RGBA BG_COLOR = { 20, 20, 20, 200 };
const RGBA HEADER_COLOR = { 30, 144, 255, 220 };
const RGBA OPTION_COLOR = { 40, 40, 40, 200 };
const RGBA SELECTED_COLOR = { 255, 255, 255, 220 };
const RGBA TEXT_COLOR = { 255, 255, 255, 255 };
const RGBA SELECTED_TEXT_COLOR = { 0, 0, 0, 255 };
const RGBA HEADER_TEXT_COLOR = { 255, 255, 255, 255 };
const RGBA TAB_BG_COLOR = { 40, 40, 40, 200 };
const RGBA SELECTED_TAB_COLOR = { 50, 160, 255, 220 };


// --- Spawn State ---
static bool spawnProtectionActive = true;
static DWORD spawnTime = 0;
static bool welcomeMessagesShown = false;
static bool g_isCustomCharacterActive = true; // Track if the custom character is currently applied
static bool waitingForRespawn = false;      // Flag to manage the respawn process for custom characters.
static Hash g_activePlayerModel = 0; // Stores the hash of the currently controlled player model

// State for handling the "busted" event
enum BustedState {
    BustedState_None = 0,
    BustedState_Initiated,
    BustedState_Recovering
};
static BustedState currentBustedState = BustedState_None;


// Forward declarations
void draw_car_shop_menu();
void draw_garage_menu();
void draw_gun_store_menu();
void draw_credits_menu();
void draw_settings_menu(); // Added this forward declaration
void LoadGameDataInitial(); // Renamed from LoadGameData to avoid confusion


// Helper function to request and load a model
bool RequestModel(Hash modelHash) {
    if (!STREAMING::IS_MODEL_IN_CDIMAGE(modelHash) || !STREAMING::IS_MODEL_VALID(modelHash)) {
        return false;
    }
    STREAMING::REQUEST_MODEL(modelHash);
    DWORD timeout = GetTickCount64() + 1000; // 1 second timeout
    while (!STREAMING::HAS_MODEL_LOADED(modelHash) && GetTickCount64() < timeout) {
        WAIT(0);
    }
    return STREAMING::HAS_MODEL_LOADED(modelHash);
}

// Native function declarations
// These assume they are available through ScriptHookV's headers (e.g., natives.h)
// The hash 0x388A47C51ABDAC8E is used to invoke the native.
static BOOL IS_PLAYER_BEING_ARRESTED(Player player, BOOL atArresting) { return invoke<BOOL>(0x388A47C51ABDAC8E, player, atArresting); }
static BOOL IS_PED_CUFFED(Ped ped) { return invoke<BOOL>(0x74E559B3BC910685, ped); }
static void UNCUFF_PED(Ped ped) { invoke<Void>(0x67406F2C8F87FC4F, ped); } // Corrected: Removed extra '0' from 00x to 0x

// --- Drawing Helper Function Definitions ---
// Restored DrawMenuHeader function
void DrawMenuHeader(const char* text, float x, float y, float w) {
    // This function now draws only the text, the background is handled by the caller
    UI::SET_TEXT_FONT(FONT);
    UI::SET_TEXT_SCALE(0.0f, 0.43f);
    UI::SET_TEXT_COLOUR(HEADER_TEXT_COLOR.r, HEADER_TEXT_COLOR.g, HEADER_TEXT_COLOR.b, HEADER_TEXT_COLOR.a);
    UI::SET_TEXT_CENTRE(true);
    UI::_SET_TEXT_ENTRY("STRING");
    UI::_ADD_TEXT_COMPONENT_STRING(const_cast<char*>(text)); // Use const_cast for string literals
    UI::_DRAW_TEXT(x + w * 0.5f, y + 0.007f); // Position text relative to the top of its row
}

void DrawMenuOption(const char* text, float x, float y, float w, float h, bool selected) {
    GRAPHICS::DRAW_RECT(x + w * 0.5f, y + (h - 0.004f) * 0.5f, w, h - 0.004f,
        selected ? SELECTED_COLOR.r : OPTION_COLOR.r,
        selected ? SELECTED_COLOR.g : OPTION_COLOR.g,
        selected ? SELECTED_COLOR.b : OPTION_COLOR.b,
        selected ? SELECTED_COLOR.a : OPTION_COLOR.a);
    UI::SET_TEXT_FONT(FONT);
    UI::SET_TEXT_SCALE(0.0f, 0.38f);
    UI::SET_TEXT_COLOUR(
        selected ? SELECTED_TEXT_COLOR.r : TEXT_COLOR.r,
        selected ? SELECTED_TEXT_COLOR.g : TEXT_COLOR.g,
        selected ? SELECTED_TEXT_COLOR.b : TEXT_COLOR.b,
        selected ? SELECTED_TEXT_COLOR.a : TEXT_COLOR.a);
    UI::SET_TEXT_CENTRE(false);
    UI::_SET_TEXT_ENTRY("STRING");
    UI::_ADD_TEXT_COMPONENT_STRING(const_cast<char*>(text)); // Use const_cast for string literals
    UI::_DRAW_TEXT(x + 0.01f, y + 0.007f);
}

void DrawPairedMenuOption(const char* label, const char* value, float x, float y, float w, float h, bool selected) {
    DrawMenuOption(label, x, y, w, h, selected);

    UI::SET_TEXT_FONT(FONT);
    UI::SET_TEXT_SCALE(0.0f, 0.38f);
    UI::SET_TEXT_COLOUR(
        selected ? SELECTED_TEXT_COLOR.r : TEXT_COLOR.r,
        selected ? SELECTED_TEXT_COLOR.g : TEXT_COLOR.g,
        selected ? SELECTED_TEXT_COLOR.b : TEXT_COLOR.b,
        selected ? SELECTED_TEXT_COLOR.a : TEXT_COLOR.a);
    UI::SET_TEXT_RIGHT_JUSTIFY(true);
    UI::SET_TEXT_WRAP(0.0f, x + w - 0.01f);
    UI::_SET_TEXT_ENTRY("STRING");
    UI::_ADD_TEXT_COMPONENT_STRING(const_cast<char*>(value)); // Use const_cast for string literals
    UI::_DRAW_TEXT(x, y + 0.007f);
    UI::SET_TEXT_RIGHT_JUSTIFY(false);
}

// --- Character Save/Load/Switch Logic ---

void SaveCurrentCharacterData() {
    if (g_isCustomCharacterActive) {
        CharacterCreator_Save(characterFile);
        Money_Save(playerStatsFile);
        RankBar_Save(playerStatsFile); // Assuming RankBar_Save takes path
        RpEvents_Save(xpFile);
        GunStore_Save(); // No path argument, uses its internal default
        Properties_Save(propertiesFile);
    }
    else {
        // Save to consolidated protagonist save files
        Money_Save(PROTAGONIST_MONEY_FILE);
        RankBar_Save(PROTAGONIST_RANK_FILE); // Assuming RankBar_Save takes path
        RpEvents_Save(PROTAGONIST_XP_FILE);
        GunStore_Save(); // No path argument, uses its internal default
        Properties_Save(PROTAGONIST_PROPERTIES_FILE);
    }

    UI::_SET_NOTIFICATION_TEXT_ENTRY("STRING");
    UI::_ADD_TEXT_COMPONENT_STRING(const_cast<char*>("Game Saved."));
    UI::_DRAW_NOTIFICATION(false, true);
}

void LoadCustomCharacterData() {
    if (!RequestModel(GAMEPLAY::GET_HASH_KEY("mp_m_freemode_01"))) { // Always load a freemode model for custom char
        UI::_SET_NOTIFICATION_TEXT_ENTRY("STRING");
        UI::_ADD_TEXT_COMPONENT_STRING(const_cast<char*>("~r~Failed to load custom character model."));
        UI::_DRAW_NOTIFICATION(false, true);
        return;
    }
    PLAYER::SET_PLAYER_MODEL(PLAYER::PLAYER_ID(), GAMEPLAY::GET_HASH_KEY("mp_m_freemode_01")); // Default to male freemode for loading
    g_activePlayerModel = GAMEPLAY::GET_HASH_KEY("mp_m_freemode_01"); // Update active model
    g_isCustomCharacterActive = true;

    CharacterCreator_Load(characterFile); // This will re-apply the correct gender/model/appearance
    Money_Load(playerStatsFile);
    RankBar_Load(playerStatsFile); // Assuming RankBar_Load takes path
    RpEvents_Load(xpFile);
    GunStore_Load(); // No path argument, uses its internal default
    Properties_Load(propertiesFile);

    CharacterCreator_Apply(); // Apply character features after loading
    UI::_SET_NOTIFICATION_TEXT_ENTRY("STRING");
    UI::_ADD_TEXT_COMPONENT_STRING(const_cast<char*>("Custom Character Loaded!"));
    UI::_DRAW_NOTIFICATION(false, true);
}

void LoadProtagonistData(Hash protagonistModelHash) {
    // This function will now ONLY load data, not change the model.
    // The protagonistModelHash argument is kept for consistency but its direct use for model setting is removed.

    // Load from consolidated protagonist save files
    Money_Load(PROTAGONIST_MONEY_FILE);
    RankBar_Load(PROTAGONIST_RANK_FILE); // Assuming RankBar_Load takes path
    RpEvents_Load(PROTAGONIST_XP_FILE);
    GunStore_Load(); // No path argument, uses its internal default
    Properties_Load(PROTAGONIST_PROPERTIES_FILE);

    // Set g_isCustomCharacterActive to false, as we're loading protagonist data
    g_isCustomCharacterActive = false;

    UI::_SET_NOTIFICATION_TEXT_ENTRY("STRING");
    UI::_ADD_TEXT_COMPONENT_STRING(const_cast<char*>("Protagonist Save Loaded!"));
    UI::_DRAW_NOTIFICATION(false, true);
}

void SwitchToCharacter(Hash newModelHash) {
    // This function is still present but not called by the current save/load menu options.
    // It's kept for potential future to switch character models without loading data.
    if (!RequestModel(newModelHash)) {
        UI::_SET_NOTIFICATION_TEXT_ENTRY("STRING");
        UI::_ADD_TEXT_COMPONENT_STRING(const_cast<char*>("~r~Failed to switch character model."));
        UI::_DRAW_NOTIFICATION(false, true);
        return;
    }
    PLAYER::SET_PLAYER_MODEL(PLAYER::PLAYER_ID(), newModelHash);
    g_activePlayerModel = newModelHash;
    WAIT(100); // Small wait to allow game to process model change

    // Apply basic model settings/clear custom
    if (newModelHash == FRANKLIN_MODEL_HASH || newModelHash == MICHAEL_MODEL_HASH || newModelHash == TREVOR_MODEL_HASH) {
        g_isCustomCharacterActive = false;
        // Clear custom character appearance and force default protagonist outfit
        PED::SET_PED_HEAD_BLEND_DATA(PLAYER::PLAYER_PED_ID(), 0, 0, 0, 0, 0, 0, 0.0f, 0.0f, 0.0f, false);
        for (int i = 0; i < 20; ++i) PED::_SET_PED_FACE_FEATURE(PLAYER::PLAYER_PED_ID(), i, 0.0f);
        for (int i = 0; i < 12; ++i) PED::SET_PED_COMPONENT_VARIATION(PLAYER::PLAYER_PED_ID(), i, 0, 0, 2);
        for (int i = 0; i < 5; ++i) PED::CLEAR_PED_PROP(PLAYER::PLAYER_PED_ID(), i);
        PED::SET_PED_DEFAULT_COMPONENT_VARIATION(PLAYER::PLAYER_PED_ID()); // Force default outfit
        PED::SET_PED_RANDOM_PROPS(PLAYER::PLAYER_PED_ID()); // Clear any random props
        PED::SET_PED_RANDOM_COMPONENT_VARIATION(PLAYER::PLAYER_PED_ID(), FALSE); // Clear any random components

        UI::_SET_NOTIFICATION_TEXT_ENTRY("STRING");
        UI::_ADD_TEXT_COMPONENT_STRING(const_cast<char*>(std::string("Switched to " + std::string(
            newModelHash == FRANKLIN_MODEL_HASH ? "Franklin" :
            newModelHash == MICHAEL_MODEL_HASH ? "Michael" :
            newModelHash == TREVOR_MODEL_HASH ? "Trevor" : ""
        ) + "!").c_str()));
        UI::_DRAW_NOTIFICATION(false, true);

    }
    else { // Assume custom character
        g_isCustomCharacterActive = true;
        CharacterCreator_Apply(); // Re-apply custom character's saved appearance
        UI::_SET_NOTIFICATION_TEXT_ENTRY("STRING");
        UI::_ADD_TEXT_COMPONENT_STRING(const_cast<char*>("Switched to Custom Character!"));
        UI::_DRAW_NOTIFICATION(false, true);
    }
    // Load data for the switched character (money, rank, etc.)
    // This part is redundant with the current menu setup, but kept if SwitchToCharacter is called elsewhere.
    Money_Load(g_isCustomCharacterActive ? playerStatsFile : PROTAGONIST_MONEY_FILE);
    RankBar_Load(g_isCustomCharacterActive ? playerStatsFile : PROTAGONIST_RANK_FILE); // Assuming RankBar_Load takes path
    RpEvents_Load(g_isCustomCharacterActive ? xpFile : PROTAGONIST_XP_FILE);
    GunStore_Load(); // No path argument, uses its internal default
    Properties_Load(g_isCustomCharacterActive ? propertiesFile : PROTAGONIST_PROPERTIES_FILE);
}


void draw_main_menu() {
    const int numOptions = 10; // Increased by 1 for Settings
    const char* labels[numOptions] = {
        "Character Creator", "Cheats", "Vehicle", "Save/Load",
        "Car Shop", "Garage", "Gun Store", "Credits", "Settings", "Close Menu" // Added "Settings"
    };

    float x = MENU_X, y = MENU_Y, w = MENU_W, h = MENU_H;

    // Calculate total height including header
    float headerHeight = MENU_H; // Height of the header bar
    float optionsTotalHeight = h * numOptions; // Total height of all options
    float totalMenuHeight = headerHeight + optionsTotalHeight;

    // Calculate the center Y for the entire menu block (header + options)
    // The top of the menu is MENU_Y. The center of the entire block will be MENU_Y + (totalMenuHeight / 2.0f)
    float menuBlockCenterY = MENU_Y + (totalMenuHeight / 2.0f);

    GRAPHICS::DRAW_RECT(x + w * 0.5f, menuBlockCenterY, w, totalMenuHeight, BG_COLOR.r, BG_COLOR.g, BG_COLOR.b, BG_COLOR.a);

    // Draw the header background (blue part)
    float headerBgCenterY = MENU_Y + (headerHeight * 0.5f); // Center Y of the header bar
    GRAPHICS::DRAW_RECT(x + w * 0.5f, headerBgCenterY, w, headerHeight, HEADER_COLOR.r, HEADER_COLOR.g, HEADER_COLOR.b, HEADER_COLOR.a);

    // Draw the header text using the shared function
    DrawMenuHeader("GTA OFFLINE", x, MENU_Y, w); // Pass MENU_Y as the top of the header row

    // Draw menu options, starting below the header
    float optionsStartY = MENU_Y + headerHeight;
    for (int i = 0; i < numOptions; ++i) {
        DrawMenuOption(labels[i], x, optionsStartY + h * i, w, h, i == menuIndex);
    }

    ClampMenuIndex(menuIndex, numOptions);

    // Only process input if the global input delay is 0
    if (inputDelayFrames == 0) {
        int up = 0, down = 0;
        if (IsKeyJustUp(VK_NUMPAD8) || PadPressed(DPAD_UP))   up = 1;
        if (IsKeyJustUp(VK_NUMPAD2) || PadPressed(DPAD_DOWN)) down = 1;

        if (up) {
            menuIndex = (menuIndex - 1 + numOptions) % numOptions;
            inputDelayFrames = 10; // Apply delay after navigation
        }
        if (down) {
            menuIndex = (menuIndex + 1) % numOptions;
            inputDelayFrames = 10; // Apply delay after navigation
        }

        static bool prevA = false;
        bool currA = PadPressed(BTN_A);
        if ((IsKeyJustUp(VK_NUMPAD5) || (currA && !prevA))) {
            switch (menuIndex) {
            case 0: menuCategory = CAT_CHARACTER; menuIndex = 0; inputDelayFrames = 10; break;
            case 1: menuCategory = CAT_CHEATS;    menuIndex = 0; inputDelayFrames = 10; break;
            case 2: menuCategory = CAT_VEHICLE;   menuIndex = 0; inputDelayFrames = 10; break;
            case 3: menuCategory = CAT_SAVELOAD;  menuIndex = 0; inputDelayFrames = 10; break;
            case 4: menuCategory = CAT_CAR_SHOP;  menuIndex = 0; inputDelayFrames = 10; break;
            case 5: menuCategory = CAT_GARAGE;    menuIndex = 0; inputDelayFrames = 10; break;
            case 6: menuCategory = CAT_GUN_STORE; menuIndex = 0; inputDelayFrames = 10; break;
            case 7: menuCategory = CAT_CREDITS;   menuIndex = 0; inputDelayFrames = 10; break;
            case 8: menuCategory = CAT_SETTINGS;  menuIndex = 0; inputDelayFrames = 10; break; // Handle Settings
            case 9: menuOpen = false;             menuIndex = 0; inputDelayFrames = 10; break; // Close Menu is now case 9
            }
        }
        prevA = currA;
    }
}


int saveloadMenuIndex = 0;
void draw_saveload_menu() {
    // Consolidated options for Save/Load
    const int numOptions = 4; // Save, Load Custom, Load Protagonist, Back
    // Base labels, will be copied to dynamicLabels
    const char* baseLabels[numOptions] = {
        "Save Current Character", // This will be dynamically changed
        "Load Custom Character",
        "Load Protagonist", // Changed from "Load Protagonist Save"
        "Back"
    };

    // Create a mutable array for labels to allow dynamic text
    char dynamicLabels[numOptions][64];
    for (int i = 0; i < numOptions; ++i) {
        strcpy_s(dynamicLabels[i], sizeof(dynamicLabels[i]), baseLabels[i]);
    }

    // Dynamically set the "Save Current Character" label
    if (g_isCustomCharacterActive) {
        strcpy_s(dynamicLabels[0], sizeof(dynamicLabels[0]), "Save Custom Character");
    }
    else {
        strcpy_s(dynamicLabels[0], sizeof(dynamicLabels[0]), "Save Protagonist"); // Changed from "Save Protagonist Progress"
    }


    float x = MENU_X, y = MENU_Y, w = MENU_W, h = MENU_H;
    // Calculate total height including header
    float headerHeight = MENU_H; // Height of the header bar
    float optionsTotalHeight = h * numOptions; // Total height of all options
    float totalMenuHeight = headerHeight + optionsTotalHeight;

    // Calculate the center Y for the entire menu block (header + options)
    float menuBlockCenterY = MENU_Y + (totalMenuHeight / 2.0f);

    GRAPHICS::DRAW_RECT(x + w * 0.5f, menuBlockCenterY, w, totalMenuHeight, BG_COLOR.r, BG_COLOR.g, BG_COLOR.b, BG_COLOR.a);

    // Draw the header background (blue part)
    float headerBgCenterY = MENU_Y + (headerHeight * 0.5f); // Center Y of the header bar
    GRAPHICS::DRAW_RECT(x + w * 0.5f, headerBgCenterY, w, headerHeight, HEADER_COLOR.r, HEADER_COLOR.g, HEADER_COLOR.b, HEADER_COLOR.a);

    // Draw the header text using the shared function
    DrawMenuHeader("SAVE / LOAD", x, MENU_Y, w); // Pass MENU_Y as the top of the header row

    // Draw menu options, starting below the header
    float optionsStartY = MENU_Y + headerHeight;
    for (int i = 0; i < numOptions; ++i) {
        DrawMenuOption(dynamicLabels[i], x, optionsStartY + h * i, w, h, i == saveloadMenuIndex);
    }

    ClampMenuIndex(saveloadMenuIndex, numOptions);

    // Only process input if the global input delay is 0
    if (inputDelayFrames == 0) {
        int up = 0, down = 0;
        if (IsKeyJustUp(VK_NUMPAD8) || PadPressed(DPAD_UP))   up = 1;
        if (IsKeyJustUp(VK_NUMPAD2) || PadPressed(DPAD_DOWN)) down = 1;

        if (up) {
            saveloadMenuIndex = (saveloadMenuIndex - 1 + numOptions) % numOptions;
            inputDelayFrames = 10; // Apply delay after navigation
        }
        if (down) {
            saveloadMenuIndex = (saveloadMenuIndex + 1) % numOptions;
            inputDelayFrames = 10; // Apply delay after navigation
        }

        static bool prevA = false;
        bool currA = PadPressed(BTN_A);
        if ((IsKeyJustUp(VK_NUMPAD5) || (currA && !prevA))) {
            switch (saveloadMenuIndex) {
            case 0: // Save Custom Character / Save Protagonist (dynamic label)
                SaveCurrentCharacterData();
                inputDelayFrames = 10;
                break;
            case 1: // Load Custom Character
                LoadCustomCharacterData();
                inputDelayFrames = 10;
                break;
            case 2: // Load Protagonist (consolidated)
                // When loading the protagonist save, we'll default to Franklin's model.
                // The actual save data (money, rank, etc.) will be the shared protagonist data.
                LoadProtagonistData(FRANKLIN_MODEL_HASH);
                inputDelayFrames = 10;
                break;
            case 3: // Back
                menuCategory = CAT_MAIN; menuIndex = 3; saveloadMenuIndex = 0; inputDelayFrames = 10; // Go back
                break;
            default:
                // This case should ideally not be reached with proper clamping
                break;
            }
        }
        prevA = currA;
    }
}

// This function is called on ScriptMain to load initial character/data
void LoadGameDataInitial() // Renamed from LoadGameData to avoid confusion
{
    // On initial load, we assume the custom character is the default.
    // If g_autoLoadCharacter is true, it will load the custom character's data.
    // If g_autoLoadCharacter is false, it will start with a default state (which will be a protagonist if not overridden).
    CharacterCreator_Load(characterFile);
    Money_Load(playerStatsFile);
    RankBar_Load(playerStatsFile);
    RpEvents_Load(xpFile);
    GunStore_Load(); // No path argument, uses its internal default
    Properties_Load(propertiesFile);
    WAIT(100); // Small delay to ensure files are read

    CharacterCreator_Apply();
    g_isCustomCharacterActive = true; // Mark custom character as active after initial load
}


void SkipIntroAndLoadCharacter()
{
    spawnTime = GetTickCount64();

    CAM::DO_SCREEN_FADE_OUT(0);
    WAIT(700);

    NETWORK::NETWORK_END_TUTORIAL_SESSION();

    Player player = PLAYER::PLAYER_ID();
    Ped playerPed = PLAYER::PLAYER_PED_ID();
    ENTITY::FREEZE_ENTITY_POSITION(playerPed, false);

    WAIT(1200);

    while (!PLAYER::IS_PLAYER_PLAYING(player)) WAIT(100);

    PLAYER::SET_PLAYER_CONTROL(player, true, 0);
    WAIT(400);

    CAM::DO_SCREEN_FADE_IN(800);

    WAIT(1000);

    // Only load game data if g_autoLoadCharacter is true
    if (g_autoLoadCharacter) {
        LoadGameDataInitial(); // Call the initial load function
    }
    else {
        // If not auto-loading, ensure custom character is marked as inactive
        g_isCustomCharacterActive = false; // Player starts as default protagonist or empty state
        UI::_SET_NOTIFICATION_TEXT_ENTRY("STRING");
        UI::_ADD_TEXT_COMPONENT_STRING(const_cast<char*>("~y~Auto-load character is OFF. Using default character."));
        UI::_DRAW_NOTIFICATION(false, true);
    }
}

void ScriptMain() {
    Cheats_Init();
    RankBar_Init();
    Money_Init();
    CharacterCreator_Init();
    RpEvents_Init();
    CarShop_Init();
    Garage_Init();
    GunStore_Init();
    Credits_Init();
    CarExport_Init();
    Properties_Init();
    Settings_Init(); // Initialize settings (color themes)
    LoadSettings();  // Load settings from file
    // ApplyColorTheme(); // Removed call to ApplyColorTheme()

    SkipIntroAndLoadCharacter();
    g_activePlayerModel = ENTITY::GET_ENTITY_MODEL(PLAYER::PLAYER_PED_ID()); // Set initial active model

    extern void (*Vehicle_DrawMenu)(int& menuIndex, int& menuCategory);
    srand((unsigned int)GetTickCount64());

    bool prevMenuCombo = false;
    bool prevB = false;

    while (true)
    {
        PollPad();

        Player player = PLAYER::PLAYER_ID();
        Ped playerPed = PLAYER::PLAYER_PED_ID();

        // --- Custom Character Respawn Logic (Death) ---
        // Player dies -> switch to generic model, prepare for respawn
        if (PLAYER::IS_PLAYER_DEAD(player)) {
            if (g_isCustomCharacterActive && !waitingForRespawn) { // Ensure this triggers only once per death
                waitingForRespawn = true; // Set flag to manage respawn

                // Immediately switch to a stable generic model. This is the primary crash prevention.
                Hash genericModel = GAMEPLAY::GET_HASH_KEY("mp_m_freemode_01");
                if (RequestModel(genericModel)) {
                    PLAYER::SET_PLAYER_MODEL(player, genericModel);
                }
                else {
                    UI::_SET_NOTIFICATION_TEXT_ENTRY("STRING");
                    UI::_ADD_TEXT_COMPONENT_STRING(const_cast<char*>("~r~Warning: Failed to load generic model for death. May lead to issues."));
                    UI::_DRAW_NOTIFICATION(false, true);
                }

                // Force game's respawn process.
                GAMEPLAY::SET_FADE_OUT_AFTER_DEATH(true); // Ensure screen fades out
                GAMEPLAY::SET_FADE_IN_AFTER_DEATH_ARREST(true); // Ensure screen fades back in after respawn

                // Reset player state to ensure control is given back to the game.
                PLAYER::SET_PLAYER_CONTROL(player, true, 0); // Give control back to game
                PLAYER::SET_PLAYER_WANTED_LEVEL(player, 0, false); // Clear wanted level (common respawn cleanup)
                PLAYER::SET_PLAYER_WANTED_LEVEL_NOW(player, true);
                ENTITY::FREEZE_ENTITY_POSITION(playerPed, false); // Unfreeze just in case
                AI::CLEAR_PED_TASKS(playerPed); // Clear any existing ped tasks
            }
        }
        // Player is alive -> check if we need to re-apply custom character
        else {
            if (waitingForRespawn) { // Only proceed if we were handling a respawn
                // Wait until the player is actually "playing" and the screen has faded in.
                // This is the most reliable way to ensure the game's internal respawn sequence is complete.
                if (PLAYER::IS_PLAYER_PLAYING(player) && CAM::IS_SCREEN_FADED_IN()) {
                    // Re-apply the custom character
                    if (g_isCustomCharacterActive) {
                        CharacterCreator_Apply();
                    }
                    // If player was protagonist, game will handle default appearance.

                    // Reset flags
                    waitingForRespawn = false;
                    spawnProtectionActive = true;
                    spawnTime = GetTickCount64();

                    // Notify user
                    UI::_SET_NOTIFICATION_TEXT_ENTRY("STRING");
                    UI::_ADD_TEXT_COMPONENT_STRING(const_cast<char*>("~g~Respawned successfully!"));
                    UI::_DRAW_NOTIFICATION(false, true);

                    // Release generic model
                    STREAMING::SET_MODEL_AS_NO_LONGER_NEEDED(GAMEPLAY::GET_HASH_KEY("mp_m_freemode_01"));
                    STREAMING::SET_MODEL_AS_NO_LONGER_NEEDED(GAMEPLAY::GET_HASH_KEY("player_g"));
                }
            }
        }
        // --- End Custom Character Respawn Logic (Death) ---

        // --- Custom Character Respawn Logic (Busted) ---
        bool currBeingArrested = IS_PLAYER_BEING_ARRESTED(player, true);
        bool currCuffed = IS_PED_CUFFED(playerPed);

        switch (currentBustedState) {
        case BustedState_None:
            // Detect if player just started being arrested and is not dead
            if (currBeingArrested && !PLAYER::IS_PLAYER_DEAD(player)) {
                if (g_isCustomCharacterActive) { // Only force model switch if custom character
                    // If in a vehicle, warp out to ensure proper arrest animation/sequence
                    if (PED::IS_PED_IN_ANY_VEHICLE(playerPed, FALSE)) {
                        Vehicle veh = PED::GET_VEHICLE_PED_IS_IN(playerPed, FALSE);
                        AI::TASK_LEAVE_VEHICLE(playerPed, veh, 16); // 16 = leave instantly
                        int waitTime = 0;
                        while (PED::IS_PED_IN_ANY_VEHICLE(playerPed, FALSE) && waitTime < 2000) {
                            WAIT(50);
                            waitTime += 50;
                        }
                    }

                    // Temporarily switch to generic model to allow game to handle arrest
                    PLAYER::SET_PLAYER_MODEL(player, GAMEPLAY::GET_HASH_KEY("player_g"));

                    // Ensure fade in after arrest
                    GAMEPLAY::SET_FADE_IN_AFTER_DEATH_ARREST(true);

                    // Clear wanted level immediately
                    PLAYER::SET_PLAYER_WANTED_LEVEL(player, 0, false);
                    PLAYER::SET_PLAYER_WANTED_LEVEL_NOW(player, true);
                    currentBustedState = BustedState_Initiated; // Transition to initiated state
                    UI::_SET_NOTIFICATION_TEXT_ENTRY("STRING");
                    UI::_ADD_TEXT_COMPONENT_STRING(const_cast<char*>("~b~You are being busted..."));
                    UI::_DRAW_NOTIFICATION(false, true);
                }
            }
            break;

        case BustedState_Initiated:
            // Wait for the player to no longer be actively arrested or cuffed, and screen to fade in
            // Also ensure the player is actually playing (i.e., not in a loading screen or transition)
            if (!currBeingArrested && !currCuffed && CAM::IS_SCREEN_FADED_IN() && PLAYER::IS_PLAYER_PLAYING(player)) {
                if (g_isCustomCharacterActive) { // Only re-apply if it was custom char
                    // Ensure the player is uncuffed before applying character if they somehow got cuffed
                    if (IS_PED_CUFFED(playerPed)) {
                        UNCUFF_PED(playerPed);
                        WAIT(100); // Small delay after uncuffing
                    }

                    // Handle bribe or confiscation
                    if (Money_Get() >= BRIBE_AMOUNT) {
                        Money_Add(-BRIBE_AMOUNT); // Deduct bribe
                        UI::_SET_NOTIFICATION_TEXT_ENTRY("STRING");
                        UI::_ADD_TEXT_COMPONENT_STRING(const_cast<char*>("~g~Bribe paid! ($5000)"));
                        UI::_DRAW_NOTIFICATION(false, true);
                    }
                    else {
                        GunStore_ClearAllBoughtWeapons(); // Clear all bought weapons from persistent storage
                        WEAPON::REMOVE_ALL_PED_WEAPONS(playerPed, false); // Remove all weapons from player's inventory
                        UI::_SET_NOTIFICATION_TEXT_ENTRY("STRING");
                        UI::_ADD_TEXT_COMPONENT_STRING(const_cast<char*>("~r~Not enough money! All your bought guns were confiscated!"));
                        UI::_DRAW_NOTIFICATION(false, true);
                    }

                    CharacterCreator_Apply();
                    spawnProtectionActive = true; // Re-enable spawn protection after being busted
                    spawnTime = GetTickCount64(); // Reset spawn time for protection
                    currentBustedState = BustedState_None; // Reset state to None
                }
            }
            // If for some reason the player died during the arrest, reset state
            if (PLAYER::IS_PLAYER_DEAD(player)) {
                currentBustedState = BustedState_None;
            }
            break;
        }
        // --- End Custom Character Respawn Logic (Busted) ---


        if (spawnProtectionActive) {
            ENTITY::SET_ENTITY_INVINCIBLE(playerPed, true);
            if (GetTickCount64() - spawnTime > 15000) {
                ENTITY::SET_ENTITY_INVINCIBLE(playerPed, false);
                UI::_SET_NOTIFICATION_TEXT_ENTRY("STRING");
                UI::_ADD_TEXT_COMPONENT_STRING(const_cast<char*>("Spawn protection has worn off."));
                UI::_DRAW_NOTIFICATION(false, true);
                spawnProtectionActive = false; // Set to false after notification
            }
        }

        if (!welcomeMessagesShown && GetTickCount64() - spawnTime > 2000) {
            UI::_SET_NOTIFICATION_TEXT_ENTRY("STRING");
            UI::_ADD_TEXT_COMPONENT_STRING(const_cast<char*>("~b~GTA Offline by CreamyPlaytime Loaded"));
            UI::_DRAW_NOTIFICATION(false, true);

            WAIT(4000);

            char keybindMsg[128];
            char keyboardKey[32];
            // Use GetKeyName from Settings.h/cpp to get readable key name
            sprintf_s(keyboardKey, "%s", GetKeyName(g_keyboardMenuKey).c_str());
            sprintf_s(keybindMsg, "Press %s (Keyboard) or %s + %s (Controller) to open the menu.",
                keyboardKey,
                GetControllerButtonName(g_controllerMenuButton1).c_str(),
                GetControllerButtonName(g_controllerMenuButton2).c_str());
            UI::_SET_NOTIFICATION_TEXT_ENTRY("STRING");
            UI::_ADD_TEXT_COMPONENT_STRING(const_cast<char*>(keybindMsg));
            UI::_DRAW_NOTIFICATION(false, true);

            welcomeMessagesShown = true;
        }

        Cheats_Tick();
        RpEvents_Tick();
        RankBar_Tick();
        CharacterCreator_Tick();
        CarShop_Tick();
        Garage_Tick();
        GunStore_Tick();
        CarExport_Tick();
        Properties_Tick();

        g_vehicleMenu.Tick();

        Money_PickupScan();

        // Use dynamic controller buttons for menu combo
        bool menuCombo = PadHeld(g_controllerMenuButton1) && PadHeld(g_controllerMenuButton2);

        // Menu Open/Close Logic (Consolidated Toggle)
        if (inputDelayFrames == 0) { // Only process menu open/close if no global input delay
            // Use dynamic keyboard key for menu toggle
            if (IsKeyJustUp(g_keyboardMenuKey) || (menuCombo && !prevMenuCombo)) {
                menuOpen = !menuOpen; // Toggle menu state
                menuIndex = 0; // Reset index when toggling
                menuCategory = CAT_MAIN; // Always go back to main category when toggling
                inputDelayFrames = 15; // Apply delay after toggling
            }

            // This logic is for going back within sub-menus or closing from main using B/Numpad0
            bool currB = PadPressed(BTN_B);
            if (menuOpen && ((currB && !prevB) || IsKeyJustUp(VK_NUMPAD0))) {
                if (menuCategory == CAT_MAIN) {
                    menuOpen = false;
                    menuIndex = 0;
                    inputDelayFrames = 10; // Apply delay after closing
                }
                else {
                    // Go back to main menu from any sub-category
                    menuCategory = CAT_MAIN;
                    menuIndex = 8; // Set menuIndex to point to 'Settings' in the main menu for consistency
                    inputDelayFrames = 10; // Apply delay after category change
                }
            }
            prevB = currB;
        }


        if (menuOpen)
        {
            CONTROLS::DISABLE_CONTROL_ACTION(0, 27, true);
            switch (menuCategory) {
            case CAT_MAIN:
                draw_main_menu();
                break;
            case CAT_CHARACTER:
                CharacterCreator_DrawMenu(menuIndex, menuCategory);
                break;
            case CAT_CHEATS:
                Cheats_DrawMenu(menuIndex, MENU_X, MENU_Y, MENU_W, MENU_H);
                break;
            case CAT_VEHICLE:
                Vehicle_DrawMenu(menuIndex, menuCategory);
                break;
            case CAT_SAVELOAD:
                draw_saveload_menu();
                break;
            case CAT_CAR_SHOP:
                draw_car_shop_menu();
                break;
            case CAT_GARAGE:
                draw_garage_menu();
                break;
            case CAT_GUN_STORE:
                draw_gun_store_menu();
                break;
            case CAT_CREDITS:
                draw_credits_menu();
                break;
            case CAT_SETTINGS: // Add this case for the settings menu
                draw_settings_menu();
                break;
            }
        }
        else {
            CarExport_Draw();
        }

        // Only draw RankBar and Money if their respective settings are enabled
        if (g_showRankBar) {
            RankBar_DrawBar();
        }
        if (g_showMoneyDisplay) {
            Money_Draw();
        }

        // Decrement input delay frames at the end of the tick
        if (inputDelayFrames > 0) inputDelayFrames--;

        WAIT(0);
    }
}
