#include "Garage.h"
#include "input.h"
#include "script.h" 
#include <vector>
#include <fstream>
#include <string>

// --- Global Variables ---
static std::vector<OwnedVehicle> g_ownedVehicles;
static int garageMenuIndex = 0;

// --- Helper Functions ---
void spawn_vehicle(Hash vehicleHash) {
    if (!STREAMING::IS_MODEL_IN_CDIMAGE(vehicleHash) || !STREAMING::IS_MODEL_A_VEHICLE(vehicleHash)) {
        return;
    }

    STREAMING::REQUEST_MODEL(vehicleHash);
    while (!STREAMING::HAS_MODEL_LOADED(vehicleHash)) {
        WAIT(0);
    }

    Ped playerPed = PLAYER::PLAYER_PED_ID();
    Vector3 coords = ENTITY::GET_ENTITY_COORDS(playerPed, true);
    float heading = ENTITY::GET_ENTITY_HEADING(playerPed);

    Vector3 spawnCoords;
    if (PATHFIND::GET_SAFE_COORD_FOR_PED(coords.x + 5.0f, coords.y + 5.0f, coords.z, true, &spawnCoords, 16)) {
        Vehicle veh = VEHICLE::CREATE_VEHICLE(vehicleHash, spawnCoords.x, spawnCoords.y, spawnCoords.z, heading, true, true);

        Blip blip = UI::ADD_BLIP_FOR_ENTITY(veh);
        UI::SET_BLIP_SPRITE(blip, 225);
        UI::SET_BLIP_COLOUR(blip, 2);
        UI::BEGIN_TEXT_COMMAND_SET_BLIP_NAME("STRING");
        UI::_ADD_TEXT_COMPONENT_STRING("Personal Vehicle");
        UI::END_TEXT_COMMAND_SET_BLIP_NAME(blip);

        PED::SET_PED_INTO_VEHICLE(playerPed, veh, -1);
        VEHICLE::SET_VEHICLE_IS_STOLEN(veh, false);

        bool found = false;
        for (size_t i = 0; i < g_ownedVehicles.size(); ++i) {
            if (g_ownedVehicles[i].hash == vehicleHash) {
                g_ownedVehicles[i].vehicle_handle = veh;
                g_ownedVehicles[i].blip = blip;
                found = true;
                break;
            }
        }
        if (!found) {
            g_ownedVehicles.push_back({ vehicleHash, blip, veh });
        }
    }

    STREAMING::SET_MODEL_AS_NO_LONGER_NEEDED(vehicleHash);
}

// --- Garage System ---
void Garage_Init() {
    Garage_Load();
}

void Garage_Tick() {
    for (size_t i = 0; i < g_ownedVehicles.size(); ++i) {
        if (g_ownedVehicles[i].blip != 0 && !ENTITY::DOES_ENTITY_EXIST(g_ownedVehicles[i].vehicle_handle)) {
            UI::REMOVE_BLIP(&g_ownedVehicles[i].blip);
            g_ownedVehicles[i].blip = 0;
            g_ownedVehicles[i].vehicle_handle = 0; // Clear the handle
        }
    }
}

bool Garage_HasVehicle(Hash vehicleHash) {
    for (size_t i = 0; i < g_ownedVehicles.size(); ++i) {
        if (g_ownedVehicles[i].hash == vehicleHash) {
            return true;
        }
    }
    return false;
}

// NEW: Function to check ownership by handle
bool Garage_IsVehicleOwned(Vehicle vehicle) {
    if (!ENTITY::DOES_ENTITY_EXIST(vehicle)) return false;
    for (const auto& ownedVeh : g_ownedVehicles) {
        if (ownedVeh.vehicle_handle == vehicle) {
            return true;
        }
    }
    return false;
}


void Garage_AddVehicle(Hash vehicleHash) {
    if (Garage_HasVehicle(vehicleHash)) {
        return;
    }
    OwnedVehicle newVeh = { vehicleHash, 0, 0 };
    g_ownedVehicles.push_back(newVeh);
    Garage_Save();
}

void Garage_Save() {
    std::ofstream file("GTAOfflineGarage.ini");
    if (file.is_open()) {
        for (size_t i = 0; i < g_ownedVehicles.size(); ++i) {
            file << g_ownedVehicles[i].hash << std::endl;
        }
        file.close();
    }
}

void Garage_Load() {
    g_ownedVehicles.clear();
    std::ifstream file("GTAOfflineGarage.ini");
    if (file.is_open()) {
        Hash hash;
        while (file >> hash) {
            OwnedVehicle ownedVeh = { hash, 0, 0 };
            g_ownedVehicles.push_back(ownedVeh);
        }
        file.close();
    }
}

void draw_garage_menu() {
    extern int menuCategory;
    extern int menuIndex;
    extern int inputDelayFrames;

    const float MENU_X = 0.02f;
    const float MENU_Y = 0.13f;
    const float MENU_W = 0.29f;
    const float MENU_H = 0.038f;

    // The number of options is the number of owned vehicles plus a "Back" button.
    const int numOptions = g_ownedVehicles.size() + 1;
    float totalHeight = MENU_H * (numOptions + 1);

    // Draw background and header using the new UI style
    GRAPHICS::DRAW_RECT(MENU_X + MENU_W * 0.5f, MENU_Y - MENU_H * 0.5f + totalHeight * 0.5f, MENU_W, totalHeight, BG_COLOR.r, BG_COLOR.g, BG_COLOR.b, BG_COLOR.a);
    DrawMenuHeader("Garage", MENU_X, MENU_Y, MENU_W);

    // --- Draw Options ---
    float optionY = MENU_Y + MENU_H;

    // Loop through all owned vehicles and draw them as options
    for (size_t i = 0; i < g_ownedVehicles.size(); ++i) {
        const char* vehicleName = UI::_GET_LABEL_TEXT(VEHICLE::GET_DISPLAY_NAME_FROM_VEHICLE_MODEL(g_ownedVehicles[i].hash));
        DrawMenuOption(vehicleName, MENU_X, optionY + MENU_H * i, MENU_W, MENU_H, i == garageMenuIndex);
    }

    // Draw the "Back" button at the end of the list
    bool backSelected = (g_ownedVehicles.size() == garageMenuIndex);
    DrawMenuOption("Back", MENU_X, optionY + MENU_H * g_ownedVehicles.size(), MENU_W, MENU_H, backSelected);

    // --- Navigation and Activation Logic ---
    // Added arrow key support
    if (IsKeyJustUp(VK_NUMPAD8) || IsKeyJustUp(VK_UP) || PadPressed(DPAD_UP)) garageMenuIndex = (garageMenuIndex - 1 + numOptions) % numOptions;
    if (IsKeyJustUp(VK_NUMPAD2) || IsKeyJustUp(VK_DOWN) || PadPressed(DPAD_DOWN)) garageMenuIndex = (garageMenuIndex + 1) % numOptions;

    // Added Enter key support AND Back/Escape for going back
    if (IsKeyJustUp(VK_NUMPAD5) || IsKeyJustUp(VK_RETURN) || PadPressed(BTN_A)) {
        if (garageMenuIndex == g_ownedVehicles.size()) {
            // "Back" button was selected
            menuCategory = 0; // Go back to the main menu
            menuIndex = 5;    // Highlight the "Garage" option
            inputDelayFrames = 10; // Apply delay after action
        }
        else {
            // A vehicle was selected, spawn it.
            spawn_vehicle(g_ownedVehicles[garageMenuIndex].hash);
            inputDelayFrames = 10; // Apply delay after action
        }
    }
    // Allow Back/Escape to go back from the garage menu
    if (IsKeyJustUp(VK_NUMPAD0) || IsKeyJustUp(VK_BACK) || IsKeyJustUp(VK_ESCAPE) || PadPressed(BTN_B)) {
        menuCategory = 0; // CAT_MAIN
        menuIndex = 5; // Index for Garage in main menu
        inputDelayFrames = 10; // Apply delay after action
    }
}