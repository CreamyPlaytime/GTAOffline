#define _CRT_SECURE_NO_WARNINGS
#include "RpEvents.h"
#include "CharacterCreator.h"
#include "RankBar.h"
#include "script.h"
#include "input.h"
#include "Money.h"
#include "Garage.h"
#include <vector>
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <map>
#include <string>
#include <fstream>
#include <sstream>
#include <limits> // Required for INT_MAX

// --- XP/Rank State ---
static int s_playerXP = 0, s_playerLevel = 1, s_xpToNext = 800;
static int s_recentRPGain = 0;
static ULONGLONG s_recentRPGainTime = 0;
static bool s_inVehicleLastFrame = false;
static int s_lastWantedLevel = 0;
static Vehicle s_lastVehicle = 0;

// Max player level is rank 8000.
const int MAX_PLAYER_LEVEL = 8000;

// XP TABLE ARRAY FOR RANKS < 99 (XP needed to go from current rank to next rank)
// Adjusted for more gradual progression
const int xpToNextLevelData[] =
{
    // Index 0 is placeholder (level starts at 1, so data[1] is for Rank 1->2)
    0,
    800,     // Rank 1 -> 2
    1000,    // Rank 2 -> 3
    1200,    // Rank 3 -> 4
    1500,    // Rank 4 -> 5
    1800,    // Rank 5 -> 6
    2200,    // Rank 6 -> 7
    2600,    // Rank 7 -> 8
    3000,    // Rank 8 -> 9
    3500,    // Rank 9 -> 10
    4000,    // Rank 10 -> 11
    4500,    // Rank 11 -> 12
    5000,    // Rank 12 -> 13
    5500,    // Rank 13 -> 14
    6000,    // Rank 14 -> 15
    6600,    // Rank 15 -> 16
    7200,    // Rank 16 -> 17
    7800,    // Rank 17 -> 18
    8500,    // Rank 18 -> 19
    9200,    // Rank 19 -> 20
    10000,   // Rank 20 -> 21
    11000,   // Rank 21 -> 22
    12000,   // Rank 22 -> 23
    13000,   // Rank 23 -> 24
    14000,   // Rank 24 -> 25
    15000,   // Rank 25 -> 26
    16000,   // Rank 26 -> 27
    17000,   // Rank 27 -> 28
    18000,   // Rank 28 -> 29
    19000,   // Rank 29 -> 30
    20000,   // Rank 30 -> 31
    21000,   // Rank 31 -> 32
    22000,   // Rank 32 -> 33
    23000,   // Rank 33 -> 34
    24000,   // Rank 34 -> 35
    25000,   // Rank 35 -> 36
    26000,   // Rank 36 -> 37
    27000,   // Rank 37 -> 38
    28000,   // Rank 38 -> 39
    29000,   // Rank 39 -> 40
    30000,   // Rank 40 -> 41
    31000,   // Rank 41 -> 42
    32000,   // Rank 42 -> 43
    33000,   // Rank 43 -> 44
    34000,   // Rank 44 -> 45
    35000,   // Rank 45 -> 46
    36000,   // Rank 46 -> 47
    37000,   // Rank 47 -> 48
    38000,   // Rank 48 -> 49
    39000,   // Rank 49 -> 50
    40000,   // Rank 50 -> 51
    41000,   // Rank 51 -> 52
    42000,   // Rank 52 -> 53
    43000,   // Rank 53 -> 54
    44000,   // Rank 54 -> 55
    45000,   // Rank 55 -> 56
    46000,   // Rank 56 -> 57
    47000,   // Rank 57 -> 58
    48000,   // Rank 58 -> 59
    49000,   // Rank 59 -> 60
    50000,   // Rank 60 -> 61
    51000,   // Rank 61 -> 62
    52000,   // Rank 62 -> 63
    53000,   // Rank 63 -> 64
    54000,   // Rank 64 -> 65
    55000,   // Rank 65 -> 66
    56000,   // Rank 66 -> 67
    57000,   // Rank 67 -> 68
    58000,   // Rank 68 -> 69
    59000,   // Rank 69 -> 70
    60000,   // Rank 70 -> 71
    61000,   // Rank 71 -> 72
    62000,   // Rank 72 -> 73
    63000,   // Rank 73 -> 74
    64000,   // Rank 74 -> 75
    65000,   // Rank 75 -> 76
    66000,   // Rank 76 -> 77
    67000,   // Rank 77 -> 78
    68000,   // Rank 78 -> 79
    69000,   // Rank 79 -> 80
    70000,   // Rank 80 -> 81
    71000,   // Rank 81 -> 82
    72000,   // Rank 82 -> 83
    73000,   // Rank 83 -> 84
    74000,   // Rank 84 -> 85
    75000,   // Rank 85 -> 86
    76000,   // Rank 86 -> 87
    77000,   // Rank 87 -> 88
    78000,   // Rank 88 -> 89
    79000,   // Rank 89 -> 90
    80000,   // Rank 90 -> 91
    81000,   // Rank 91 -> 92
    82000,   // Rank 92 -> 93
    83000,   // Rank 93 -> 94
    84000,   // Rank 94 -> 95
    85000,   // Rank 95 -> 96
    86000,   // Rank 96 -> 97
    87000,   // Rank 97 -> 98
    88000    // Rank 98 -> 99
};

// --- Car Delivery System ---
static const float DELIVERY_ZONE_RADIUS = 15.0f;
static std::vector<Vehicle> s_deliveredVehicles;
static Blip s_deliveryBlip = 0;
// --- NEW: Cooldown and Sale State Variables ---
static ULONGLONG s_lastDeliveryTime = 0;
static bool s_canSellVehicle = false;
static int s_potentialPayout = 0;


// ----- Safe Save/Load Logic -----
void RpEvents_Save(const char* path)
{
    std::map<std::string, std::string> data;
    std::ifstream infile(path);
    std::string line;

    if (infile.is_open()) {
        while (std::getline(infile, line)) {
            size_t pos = line.find('=');
            if (pos != std::string::npos) {
                std::string key = line.substr(0, pos);
                std::string value = line.substr(pos + 1);
                data[key] = value;
            }
        }
        infile.close();
    }

    data["xp"] = std::to_string(s_playerXP);
    data["level"] = std::to_string(s_playerLevel);
    data["xpToNext"] = std::to_string(s_xpToNext);

    std::ofstream outfile(path, std::ios::trunc);
    if (outfile.is_open()) {
        for (const auto& pair : data) {
            outfile << pair.first << "=" << pair.second << std::endl;
        }
        outfile.close();
    }
}

void RpEvents_Load(const char* path)
{
    FILE* f = fopen(path, "r");
    if (!f) return;
    char line[128];
    while (fgets(line, sizeof(line), f)) {
        char* eq = strchr(line, '='); if (!eq) continue; *eq = 0; char* val = eq + 1;
        if (strcmp(line, "xp") == 0) s_playerXP = atoi(val);
        else if (strcmp(line, "level") == 0) s_playerLevel = atoi(val);
        else if (strcmp(line, "xpToNext") == 0) s_xpToNext = atoi(val);
    }
    fclose(f);

    // --- Validate loaded values and correct if out of range ---
    if (s_playerLevel < 1) s_playerLevel = 1; // Minimum level is 1
    if (s_playerLevel > MAX_PLAYER_LEVEL) s_playerLevel = MAX_PLAYER_LEVEL; // Cap at max level

    // Recalculate xpToNext based on loaded level to ensure it's always valid
    // This handles cases where xpToNext might be corrupted or not match the level curve.
    if (s_playerLevel >= 1 && s_playerLevel < (sizeof(xpToNextLevelData) / sizeof(xpToNextLevelData[0]))) {
        s_xpToNext = xpToNextLevelData[s_playerLevel];
    }
    else if (s_playerLevel >= (sizeof(xpToNextLevelData) / sizeof(xpToNextLevelData[0]))) { // For level 99 and above
        s_xpToNext = (int)((float)s_playerLevel * 1000.0f + 50000.0f);
        // Ensure it doesn't overflow or become too small/large unexpectedly
        if (s_xpToNext <= 0) s_xpToNext = 100000; // Smallest positive default
        if (s_xpToNext > INT_MAX / 2) s_xpToNext = INT_MAX / 2; // Prevent overflow
    }
    else { // Fallback for any unexpected level value, should not be reached with validation above
        s_xpToNext = 800; // Default for level 1 (corresponds to xpToNextLevelData[1])
    }
    if (s_playerXP < 0) s_playerXP = 0; // XP cannot be negative
    if (s_playerXP >= s_xpToNext && s_playerLevel < MAX_PLAYER_LEVEL) {
        // If loaded XP is already enough for next level but not max level,
        // it means a level-up might have been missed or data is slightly off.
        // For now, cap current XP to just below next level to avoid an immediate level-up loop on load.
        // The normal RpEvents_Reward logic will handle the actual level-up next tick if player gains more RP.
        s_playerXP = s_xpToNext - 1;
    }
}

// ----- XP/Level get/set helpers -----
int RpEvents_GetXP() { return s_playerXP; }
int RpEvents_GetLevel() { return s_playerLevel; }
int RpEvents_GetXPToNext() { return s_xpToNext; }
int RpEvents_RecentRPGain() { return s_recentRPGain; }
ULONGLONG RpEvents_RecentRPGainTime() { return s_recentRPGainTime; }
void RpEvents_SetXP(int v) { s_playerXP = v; }
void RpEvents_SetLevel(int v) { s_playerLevel = v; }
void RpEvents_SetXPToNext(int v) { s_xpToNext = v; }


// ----- Core XP Reward logic + Leveling System -----
void RpEvents_Reward(int amount, const char* msg)
{
    s_playerXP += amount;
    s_recentRPGain = amount;
    s_recentRPGainTime = GetTickCount64();

    while (s_playerLevel < MAX_PLAYER_LEVEL && s_playerXP >= s_xpToNext)
    {
        s_playerXP -= s_xpToNext;
        s_playerLevel++;

        // Calculate s_xpToNext using a more gradual scaling
        if (s_playerLevel >= 1 && s_playerLevel < (sizeof(xpToNextLevelData) / sizeof(xpToNextLevelData[0])))
        {
            // Use lookup table for levels 1 to 98 (index s_playerLevel corresponds to XP for s_playerLevel -> s_playerLevel+1)
            s_xpToNext = xpToNextLevelData[s_playerLevel];
        }
        else // For levels 99 and above
        {
            // A more linear scaling formula: XP = current_level * 1000 + 50000
            // This is more gradual than the original quadratic function for higher levels.
            s_xpToNext = (int)((float)s_playerLevel * 1000.0f + 50000.0f);

            // Safeguard against extreme values (unlikely with this formula but good practice)
            if (s_xpToNext <= 0) s_xpToNext = 100000; // Ensure a positive value
            if (s_xpToNext > INT_MAX / 2) s_xpToNext = INT_MAX / 2; // Prevent overflow
        }

        UI::_SET_NOTIFICATION_TEXT_ENTRY("STRING");
        UI::_ADD_TEXT_COMPONENT_STRING("~y~LEVEL UP!");
        UI::_DRAW_NOTIFICATION(false, false);
    }
    // If player reaches MAX_PLAYER_LEVEL, ensure XP is capped and xpToNext prevents further level-ups
    if (s_playerLevel >= MAX_PLAYER_LEVEL) {
        s_playerXP = 0; // Reset current XP, player is maxed out
        s_xpToNext = INT_MAX; // Set XP to next level to max to prevent further level-up notifications
    }

    if (msg)
    {
        UI::_SET_NOTIFICATION_TEXT_ENTRY("STRING");
        UI::_ADD_TEXT_COMPONENT_STRING((char*)msg);
        UI::_DRAW_NOTIFICATION(false, false);
    }
}

// --- Car Delivery System ---
void RpEvents_CarDeliveryCheck() {
    Ped playerPed = PLAYER::PLAYER_PED_ID();
    Vector3 playerPos = ENTITY::GET_ENTITY_COORDS(playerPed, true);

    float deliveryX = 895.0f;
    float deliveryY = -2350.0f;
    float groundZ;

    if (GAMEPLAY::GET_GROUND_Z_FOR_3D_COORD(deliveryX, deliveryY, 1000.0f, &groundZ, false))
    {
        float distanceToZone = GAMEPLAY::GET_DISTANCE_BETWEEN_COORDS(playerPos.x, playerPos.y, playerPos.z, deliveryX, deliveryY, groundZ, true);

        ULONGLONG currentTime = GetTickCount64();
        bool onCooldown = (s_lastDeliveryTime != 0 && currentTime - s_lastDeliveryTime < 60000);

        // --- Marker and Cooldown Notification Logic ---
        if (onCooldown) {
            if (distanceToZone < DELIVERY_ZONE_RADIUS && PED::IS_PED_IN_ANY_VEHICLE(playerPed, false)) {
                static ULONGLONG lastCooldownNotifyTime = 0;
                if (currentTime - lastCooldownNotifyTime > 5000) {
                    char msg[128];
                    int secondsLeft = (60000 - (currentTime - s_lastDeliveryTime)) / 1000;
                    sprintf(msg, "Please wait ~r~%d~s~ seconds for the next delivery.", secondsLeft + 1);
                    UI::_SET_NOTIFICATION_TEXT_ENTRY("STRING");
                    UI::_ADD_TEXT_COMPONENT_STRING(msg);
                    UI::_DRAW_NOTIFICATION(false, false);
                    lastCooldownNotifyTime = currentTime;
                }
            }
        }
        else {
            if (distanceToZone < 150.0f) {
                GRAPHICS::DRAW_MARKER(1, deliveryX, deliveryY, groundZ - 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, DELIVERY_ZONE_RADIUS, DELIVERY_ZONE_RADIUS, 2.0f, 255, 255, 0, 100, false, true, 2, false, NULL, NULL, false);
            }
        }

        // --- Sale Confirmation and Execution Logic ---
        bool canSellNow = false;
        Vehicle currentVeh = 0;

        if (!onCooldown && distanceToZone < DELIVERY_ZONE_RADIUS && PED::IS_PED_IN_ANY_VEHICLE(playerPed, false)) {
            currentVeh = PED::GET_VEHICLE_PED_IS_IN(playerPed, false);

            // --- FIXED: Check if the vehicle is a personal vehicle ---
            if (Garage_IsVehicleOwned(currentVeh)) {
                static ULONGLONG lastOwnedNotifyTime = 0;
                if (currentTime - lastOwnedNotifyTime > 5000) { // Notify every 5 seconds
                    UI::_SET_NOTIFICATION_TEXT_ENTRY("STRING");
                    UI::_ADD_TEXT_COMPONENT_STRING("You cannot sell your personal vehicle.");
                    UI::_DRAW_NOTIFICATION(false, false);
                    lastOwnedNotifyTime = currentTime;
                }
            }
            else {
                // --- Original Sale Logic (if not a personal vehicle) ---
                int vehClass = VEHICLE::GET_VEHICLE_CLASS(currentVeh);
                if (ENTITY::DOES_ENTITY_EXIST(currentVeh) &&
                    std::find(s_deliveredVehicles.begin(), s_deliveredVehicles.end(), currentVeh) == s_deliveredVehicles.end() &&
                    vehClass != 13 && vehClass != 14 && vehClass != 15 && vehClass != 16 && vehClass != 18 && vehClass != 21)
                {
                    canSellNow = true;
                    s_potentialPayout = 25000 + (rand() % 75001);
                }
            }
        }


        if (canSellNow) {
            // Draw the confirmation prompt
            char sellMsg[128];
            sprintf(sellMsg, "Press ~g~Enter~s~ or ~g~(A)~s~ to sell vehicle for ~g~$%d~s~", s_potentialPayout);
            UI::SET_TEXT_FONT(0);
            UI::SET_TEXT_SCALE(0.0, 0.4f);
            UI::SET_TEXT_COLOUR(255, 255, 255, 255);
            UI::SET_TEXT_WRAP(0.0, 1.0);
            UI::SET_TEXT_CENTRE(1);
            UI::SET_TEXT_DROPSHADOW(2, 0, 0, 0, 255);
            UI::_SET_TEXT_ENTRY("STRING");
            UI::_ADD_TEXT_COMPONENT_STRING(sellMsg);
            UI::_DRAW_TEXT(0.5, 0.85);

            // Check for confirmation input
            if (IsKeyJustUp(VK_RETURN) || PadPressed(BTN_A)) {
                Money_Add(s_potentialPayout);
                char payoutMsg[128];
                sprintf(payoutMsg, "~g~Vehicle Delivered. Earned $%d.", s_potentialPayout);
                RpEvents_Reward(150, payoutMsg);

                s_deliveredVehicles.push_back(currentVeh);
                s_lastDeliveryTime = GetTickCount64();

                // Delete the vehicle after sale
                ENTITY::SET_ENTITY_AS_MISSION_ENTITY(currentVeh, false, true);
                ENTITY::DELETE_ENTITY(&currentVeh);
            }
        }
    }
}


// ----- RP Events Tick: Call every frame -----
void RpEvents_Tick()
{
    Ped playerPed = PLAYER::PLAYER_PED_ID();

    bool nowInVehicle = PED::IS_PED_IN_ANY_VEHICLE(playerPed, false);
    if (nowInVehicle && !s_inVehicleLastFrame) {
        Vehicle veh = PED::GET_VEHICLE_PED_IS_IN(playerPed, false);
        if (veh != 0 && veh != s_lastVehicle && !Garage_IsVehicleOwned(veh)) {
            if (VEHICLE::GET_PED_IN_VEHICLE_SEAT(veh, -1) == playerPed) {
                if ((rand() % 100) < 15) {
                    int cashFound = 50 + (rand() % 451);
                    Money_Add(cashFound);
                    char msg[64];
                    sprintf(msg, "~g~Found $%d in the glovebox.", cashFound);
                    RpEvents_Reward(20, msg);
                }
                else {
                    RpEvents_Reward(10, "~b~RP: Stole Car");
                }
            }
            s_lastVehicle = veh;
        }
    }
    s_inVehicleLastFrame = nowInVehicle;

    int wanted = PLAYER::GET_PLAYER_WANTED_LEVEL(PLAYER::PLAYER_ID());
    if (s_lastWantedLevel > 0 && wanted == 0)
        RpEvents_Reward(20 * s_lastWantedLevel, "~p~RP: Evaded Cops");
    s_lastWantedLevel = wanted;

    RpEvents_CarDeliveryCheck();
}

// ----- Module Initialization -----
void RpEvents_Init()
{
    s_playerXP = 0;
    s_playerLevel = 1;
    // Initial XP to next level for level 1 (updated for new scaling)
    s_xpToNext = xpToNextLevelData[1];
    s_recentRPGain = 0;
    s_recentRPGainTime = 0;
    s_inVehicleLastFrame = false;
    s_lastWantedLevel = 0;
    s_lastVehicle = 0;
    s_deliveredVehicles.clear();
    s_lastDeliveryTime = 0; // Initialize cooldown timer

    // The RpEvents_Load function will set the correct s_xpToNext based on s_playerLevel if a save exists.
    // Otherwise, it will remain at the initial value set above.

    if (s_deliveryBlip != 0 && UI::DOES_BLIP_EXIST(s_deliveryBlip)) {
        UI::REMOVE_BLIP(&s_deliveryBlip);
    }

    s_deliveryBlip = UI::ADD_BLIP_FOR_COORD(895.0f, -2350.0f, 30.0f);

    UI::SET_BLIP_SPRITE(s_deliveryBlip, 225); // Yellow car icon
    UI::SET_BLIP_COLOUR(s_deliveryBlip, 5); // Yellow
    UI::SET_BLIP_AS_SHORT_RANGE(s_deliveryBlip, true);
    UI::BEGIN_TEXT_COMMAND_SET_BLIP_NAME("STRING");
    UI::_ADD_TEXT_COMPONENT_STRING("Vehicle Delivery");
    UI::END_TEXT_COMMAND_SET_BLIP_NAME(s_deliveryBlip);
}

// ----- Draw rank bar (call in your HUD code) -----
void RpEvents_DrawBar()
{
    RankBar_DrawBar();
}