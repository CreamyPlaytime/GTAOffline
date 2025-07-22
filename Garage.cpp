#include "Garage.h"
#include "input.h"
#include "script.h" 
#include <vector>
#include <fstream>
#include <string>
// #include <algorithm> // Removed: No longer needed after replacing std::min
// Removed: #include <chrono>
// Removed: #include <thread>

// --- Global Variables ---
static std::vector<OwnedVehicle> g_ownedVehicles;
static int garageMenuIndex = 0;
static int currentPage = 0; // New: Current page for pagination
const int ITEMS_PER_PAGE = 10; // New: Number of cars to display per page

// --- Helper Functions ---
void spawn_vehicle(Hash vehicleHash) {
    if (!STREAMING::IS_MODEL_IN_CDIMAGE(vehicleHash) || !STREAMING::IS_MODEL_A_VEHICLE(vehicleHash)) {
        return;
    }

    STREAMING::REQUEST_MODEL(vehicleHash);
    while (!STREAMING::HAS_MODEL_LOADED(vehicleHash)) {
        WAIT(0); // Use ScriptHookV's WAIT
    }

    Ped playerPed = PLAYER::PLAYER_PED_ID();
    Vector3 playerCoords = ENTITY::GET_ENTITY_COORDS(playerPed, true);

    float spawnHeading = 0.0f;
    Vector3 spawnCoords;
    // Corrected call: pass pointer to Vector3 and ensure correct number of arguments
    BOOL foundSpot = PATHFIND::GET_CLOSEST_VEHICLE_NODE_WITH_HEADING(playerCoords.x, playerCoords.y, playerCoords.z, &spawnCoords, &spawnHeading, 1, 3.0f, 0);

    // Fallback if a vehicle node isn't found, use player's current location with offset
    if (!foundSpot) {
        // Attempt to find a safe spot for a ped, then create vehicle there.
        // This is a less ideal fallback for vehicle spawning but better than nothing.
        Vector3 tempPedSpawnCoords;
        if (PATHFIND::GET_SAFE_COORD_FOR_PED(playerCoords.x + 5.0f, playerCoords.y + 5.0f, playerCoords.z, true, &tempPedSpawnCoords, 16)) {
            spawnCoords = tempPedSpawnCoords;
        }
        else {
            // Default to player's coords if all else fails
            spawnCoords = playerCoords;
        }
        spawnHeading = ENTITY::GET_ENTITY_HEADING(playerPed);
    }

    Vehicle veh = VEHICLE::CREATE_VEHICLE(vehicleHash, spawnCoords.x, spawnCoords.y, spawnCoords.z, spawnHeading, true, true);

    // Apply temporary invincibility for 5 seconds (5000ms)
    ENTITY::SET_ENTITY_INVINCIBLE(veh, TRUE);
    DWORD invincibleEndTime = GAMEPLAY::GET_GAME_TIMER() + 5000;

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
            g_ownedVehicles[i].invincible_until = invincibleEndTime; // Set invincible end time
            found = true;
            break;
        }
    }
    if (!found) {
        g_ownedVehicles.push_back({ vehicleHash, blip, veh, invincibleEndTime }); // Include invincible_until
    }

    STREAMING::SET_MODEL_AS_NO_LONGER_NEEDED(vehicleHash);
}

// --- Garage System ---
void Garage_Init() {
    Garage_Load();
}

void Garage_Tick() {
    DWORD currentTime = GAMEPLAY::GET_GAME_TIMER();
    for (size_t i = 0; i < g_ownedVehicles.size(); ++i) {
        if (g_ownedVehicles[i].blip != 0 && !ENTITY::DOES_ENTITY_EXIST(g_ownedVehicles[i].vehicle_handle)) {
            UI::REMOVE_BLIP(&g_ownedVehicles[i].blip);
            g_ownedVehicles[i].blip = 0;
            g_ownedVehicles[i].vehicle_handle = 0; // Clear the handle
            g_ownedVehicles[i].invincible_until = 0; // Reset invincibility
        }
        // Check and disable invincibility if time has passed
        if (g_ownedVehicles[i].vehicle_handle != 0 && g_ownedVehicles[i].invincible_until != 0 && currentTime > g_ownedVehicles[i].invincible_until) {
            ENTITY::SET_ENTITY_INVINCIBLE(g_ownedVehicles[i].vehicle_handle, FALSE);
            g_ownedVehicles[i].invincible_until = 0; // Mark as no longer invincible
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
    OwnedVehicle newVeh = { vehicleHash, 0, 0, 0 }; // Initialize invincible_until to 0
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
            OwnedVehicle ownedVeh = { hash, 0, 0, 0 }; // Initialize invincible_until to 0 on load
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

    // Calculate total pages
    int totalVehicles = g_ownedVehicles.size();
    int totalPages = (totalVehicles + ITEMS_PER_PAGE - 1) / ITEMS_PER_PAGE; // Ceiling division

    // Determine the number of options on the current page
    int numOptionsOnPage = 0;
    int startIndex = currentPage * ITEMS_PER_PAGE;
    // Replaced std::min with a ternary operator
    int endIndex = (startIndex + ITEMS_PER_PAGE < totalVehicles) ? (startIndex + ITEMS_PER_PAGE) : totalVehicles;
    numOptionsOnPage = endIndex - startIndex;

    // Initialize total menu options (vehicles + Back + Prev/Next buttons)
    int totalMenuOptions = numOptionsOnPage + 1; // +1 for "Back" button
    if (currentPage > 0) { // If not on the first page, add "Previous Page"
        totalMenuOptions++;
    }
    if (currentPage < totalPages - 1) { // If not on the last page, add "Next Page"
        totalMenuOptions++;
    }

    float totalHeight = MENU_H * (totalMenuOptions + 1); // +1 for header spacing

    // Draw background and header using the new UI style
    GRAPHICS::DRAW_RECT(MENU_X + MENU_W * 0.5f, MENU_Y - MENU_H * 0.5f + totalHeight * 0.5f, MENU_W, totalHeight, BG_COLOR.r, BG_COLOR.g, BG_COLOR.b, BG_COLOR.a);
    DrawMenuHeader("Garage", MENU_X, MENU_Y, MENU_W);

    // --- Draw Options ---
    float optionY = MENU_Y + MENU_H;
    int currentOptionIndex = 0; // Tracks the index within the displayed menu

    // Draw "Previous Page" button if applicable
    if (currentPage > 0) {
        DrawMenuOption("Previous Page", MENU_X, optionY + MENU_H * currentOptionIndex, MENU_W, MENU_H, garageMenuIndex == currentOptionIndex);
        currentOptionIndex++;
    }

    // Loop through owned vehicles for the current page and draw them as options
    for (int i = startIndex; i < endIndex; ++i) {
        const char* vehicleName = UI::_GET_LABEL_TEXT(VEHICLE::GET_DISPLAY_NAME_FROM_VEHICLE_MODEL(g_ownedVehicles[i].hash));
        DrawMenuOption(vehicleName, MENU_X, optionY + MENU_H * currentOptionIndex, MENU_W, MENU_H, garageMenuIndex == currentOptionIndex);
        currentOptionIndex++;
    }

    // Draw "Next Page" button if applicable
    if (currentPage < totalPages - 1) {
        DrawMenuOption("Next Page", MENU_X, optionY + MENU_H * currentOptionIndex, MENU_W, MENU_H, garageMenuIndex == currentOptionIndex);
        currentOptionIndex++;
    }

    // Draw the "Back" button at the end of the list
    DrawMenuOption("Back", MENU_X, optionY + MENU_H * currentOptionIndex, MENU_W, MENU_H, garageMenuIndex == currentOptionIndex);
    currentOptionIndex++; // Increment for the "Back" button

    // --- Navigation and Activation Logic ---
    // Added arrow key support
    if (IsKeyJustUp(VK_NUMPAD8) || IsKeyJustUp(VK_UP) || PadPressed(DPAD_UP)) {
        garageMenuIndex = (garageMenuIndex - 1 + totalMenuOptions) % totalMenuOptions;
    }
    if (IsKeyJustUp(VK_NUMPAD2) || IsKeyJustUp(VK_DOWN) || PadPressed(DPAD_DOWN)) {
        garageMenuIndex = (garageMenuIndex + 1) % totalMenuOptions;
    }

    // Added Enter key support AND Back/Escape for going back
    if (IsKeyJustUp(VK_NUMPAD5) || IsKeyJustUp(VK_RETURN) || PadPressed(BTN_A)) {
        int selectedOptionAbsoluteIndex = 0; // This will map the menuIndex to the actual vehicle index or action

        // Handle "Previous Page"
        if (currentPage > 0 && garageMenuIndex == 0) {
            currentPage--;
            garageMenuIndex = 0; // Reset menu index to the first item on the new page
            inputDelayFrames = 10;
            return;
        }

        // Adjust selectedOptionAbsoluteIndex based on "Previous Page" button presence
        int vehicleStartIndexInMenu = (currentPage > 0) ? 1 : 0;

        // Handle vehicle selection
        if (garageMenuIndex >= vehicleStartIndexInMenu && garageMenuIndex < vehicleStartIndexInMenu + numOptionsOnPage) {
            int selectedVehicleRelativeIndex = garageMenuIndex - vehicleStartIndexInMenu;
            int actualVehicleIndex = startIndex + selectedVehicleRelativeIndex;
            spawn_vehicle(g_ownedVehicles[actualVehicleIndex].hash);
            inputDelayFrames = 10;
        }
        // Handle "Next Page"
        else if (currentPage < totalPages - 1 && garageMenuIndex == (vehicleStartIndexInMenu + numOptionsOnPage)) {
            currentPage++;
            garageMenuIndex = 0; // Reset menu index to the first item on the new page
            inputDelayFrames = 10;
            return;
        }
        // Handle "Back" button
        else if (garageMenuIndex == (totalMenuOptions - 1)) {
            menuCategory = 0; // Go back to the main menu
            menuIndex = 5;    // Highlight the "Garage" option
            currentPage = 0;  // Reset page when exiting garage
            garageMenuIndex = 0; // Reset garage menu index
            inputDelayFrames = 10;
        }
    }
    // Allow Back/Escape to go back from the garage menu
    if (IsKeyJustUp(VK_NUMPAD0) || IsKeyJustUp(VK_BACK) || IsKeyJustUp(VK_ESCAPE) || PadPressed(BTN_B)) {
        menuCategory = 0; // CAT_MAIN
        menuIndex = 5; // Index for Garage in main menu
        currentPage = 0; // Reset page when exiting garage
        garageMenuIndex = 0; // Reset garage menu index
        inputDelayFrames = 10; // Apply delay after action
    }
}