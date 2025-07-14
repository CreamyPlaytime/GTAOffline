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
#include <limits>

// --- XP/Rank State ---
static int s_playerXP = 0, s_playerLevel = 1, s_xpToNext = 800;
static int s_recentRPGain = 0;
static ULONGLONG s_recentRPGainTime = 0;
static bool s_inVehicleLastFrame = false;
static int s_lastWantedLevel = 0;
static Vehicle s_lastVehicle = 0;

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

// XP TABLE ARRAY FOR RANKS < 100
const int xpToNextLevelData[] = 
{
    0, // Placeholder for index 0 or XP to reach Rank 1
    800, // XP to go from Rank 1 to Rank 2
    2100, // XP to go from Rank 2 to Rank 3
    3800, // XP to go from Rank 3 to Rank 4
    6100, // XP to go from Rank 4 to Rank 5
    9500, // XP to go from Rank 5 to Rank 6
    12500, // XP to go from Rank 6 to Rank 7
    16000, // XP to go from Rank 7 to Rank 8
    19800, // XP to go from Rank 8 to Rank 9
    24000, // XP to go from Rank 9 to Rank 10
    28500, // XP to go from Rank 10 to Rank 11
    33400, // XP to go from Rank 11 to Rank 12
    38700, // XP to go from Rank 12 to Rank 13
    44200, // XP to go from Rank 13 to Rank 14
    50200, // XP to go from Rank 14 to Rank 15
    56400, // XP to go from Rank 15 to Rank 16
    63000, // XP to go from Rank 16 to Rank 17
    69900, // XP to go from Rank 17 to Rank 18
    77100, // XP to go from Rank 18 to Rank 19
    84700, // XP to go from Rank 19 to Rank 20
    92500, // XP to go from Rank 20 to Rank 21
    100700, // XP to go from Rank 21 to Rank 22
    109200, // XP to go from Rank 22 to Rank 23
    118000, // XP to go from Rank 23 to Rank 24
    127100, // XP to go from Rank 24 to Rank 25
    136500, // XP to go from Rank 25 to Rank 26
    146200, // XP to go from Rank 26 to Rank 27
    156200, // XP to go from Rank 27 to Rank 28
    166500, // XP to go from Rank 28 to Rank 29
    177100, // XP to go from Rank 29 to Rank 30
    188000, // XP to go from Rank 30 to Rank 31
    199200, // XP to go from Rank 31 to Rank 32
    210700, // XP to go from Rank 32 to Rank 33
    224000, // XP to go from Rank 33 to Rank 34
    234500, // XP to go from Rank 34 to Rank 35
    246800, // XP to go from Rank 35 to Rank 36
    259400, // XP to go from Rank 36 to Rank 37
    272300, // XP to go from Rank 37 to Rank 38
    285500, // XP to go from Rank 38 to Rank 39
    299000, // XP to go from Rank 39 to Rank 40
    312700, // XP to go from Rank 40 to Rank 41
    326800, // XP to go from Rank 41 to Rank 42
    341000, // XP to go from Rank 42 to Rank 43
    355600, // XP to go from Rank 43 to Rank 44
    370500, // XP to go from Rank 44 to Rank 45
    385600, // XP to go from Rank 45 to Rank 46
    401000, // XP to go from Rank 46 to Rank 47
    416600, // XP to go from Rank 47 to Rank 48
    432600, // XP to go from Rank 48 to Rank 49
    448800, // XP to go from Rank 49 to Rank 50
    465200, // XP to go from Rank 50 to Rank 51
    482000, // XP to go from Rank 51 to Rank 52
    499000, // XP to go from Rank 52 to Rank 53
    516300, // XP to go from Rank 53 to Rank 54
    533800, // XP to go from Rank 54 to Rank 55
    551600, // XP to go from Rank 55 to Rank 56
    569600, // XP to go from Rank 56 to Rank 57
    588000, // XP to go from Rank 57 to Rank 58
    606500, // XP to go from Rank 58 to Rank 59
    625400, // XP to go from Rank 59 to Rank 60
    644500, // XP to go from Rank 60 to Rank 61
    663800, // XP to go from Rank 61 to Rank 62
    683400, // XP to go from Rank 62 to Rank 63
    703300, // XP to go from Rank 63 to Rank 64
    723400, // XP to go from Rank 64 to Rank 65
    743800, // XP to go from Rank 65 to Rank 66
    764500, // XP to go from Rank 66 to Rank 67
    785400, // XP to go from Rank 67 to Rank 68
    806500, // XP to go from Rank 68 to Rank 69
    827900, // XP to go from Rank 69 to Rank 70
    849600, // XP to go from Rank 70 to Rank 71
    871500, // XP to go from Rank 71 to Rank 72
    893600, // XP to go from Rank 72 to Rank 73
    916000, // XP to go from Rank 73 to Rank 74
    938700, // XP to go from Rank 74 to Rank 75
    961600, // XP to go from Rank 75 to Rank 76
    984700, // XP to go from Rank 76 to Rank 77
    1008100, // XP to go from Rank 77 to Rank 78
    1031800, // XP to go from Rank 78 to Rank 79
    1055700, // XP to go from Rank 79 to Rank 80
    1079800, // XP to go from Rank 80 to Rank 81
    1104200, // XP to go from Rank 81 to Rank 82
    1128800, // XP to go from Rank 82 to Rank 83
    1153700, // XP to go from Rank 83 to Rank 84
    1178800, // XP to go from Rank 84 to Rank 85
    1204200, // XP to go from Rank 85 to Rank 86
    1229800, // XP to go from Rank 86 to Rank 87
    1255600, // XP to go from Rank 87 to Rank 88
    1281700, // XP to go from Rank 88 to Rank 89
    1308100, // XP to go from Rank 89 to Rank 90
    1334600, // XP to go from Rank 90 to Rank 91
    1361400, // XP to go from Rank 91 to Rank 92
    1388500, // XP to go from Rank 92 to Rank 93
    1415800, // XP to go from Rank 93 to Rank 94
    1443300, // XP to go from Rank 94 to Rank 95
    1471100, // XP to go from Rank 95 to Rank 96
    1499100, // XP to go from Rank 96 to Rank 97
    1527300, // XP to go from Rank 97 to Rank 98
    1555800  // XP to go from Rank 98 to Rank 99
};

// Max player level is rank 8000.
const int MAX_PLAYER_LEVEL = 8000;

// ----- Core XP Reward logic + Leveling System -----
void RpEvents_Reward(int amount, const char* msg) 
{
    s_playerXP += amount;
    s_recentRPGain = amount;
    s_recentRPGainTime = GetTickCount64();

    while (s_playerXP >= s_xpToNext) 
    {
        // This block is crucial for capping the level
        if (s_playerLevel >= MAX_PLAYER_LEVEL) 
        {
            s_playerXP = 0; // Optionally reset current XP
            s_xpToNext = INT_MAX; // Prevent further rank-ups (requires #include <limits>)
            break; // Exit the loop
        }

        s_playerXP -= s_xpToNext;
        s_playerLevel++;

        // Check if the player level is 99 or higher
        if (s_playerLevel > 98) 
        {
            // Calculate s_xpToNext using function f(x) = 25x^2 + 23575x - 1023150
            // where x is the current rank -> "s_playerLevel".
            s_xpToNext = (25 * s_playerLevel * s_playerLevel) + (23575 * s_playerLevel) - 1023150;
        } 
        else 
        {
            // For levels 1 to 99, XP needed to rank up will be acquired from the lookup table.
            if (s_playerLevel >= 1 && s_playerLevel < (sizeof(xpToNextLevelData) / sizeof(xpToNextLevelData[0]))) 
            {
                s_xpToNext = xpToNextLevelData[s_playerLevel];
            }
        }
        
        UI::_SET_NOTIFICATION_TEXT_ENTRY("STRING");
        UI::_ADD_TEXT_COMPONENT_STRING("~y~LEVEL UP!");
        UI::_DRAW_NOTIFICATION(false, false);
    }
    
    if (msg) 
    {
        UI::_SET_NOTIFICATION_TEXT_ENTRY("STRING");
        UI::_ADD_TEXT_COMPONENT_STRING((char*)msg);
        UI::_DRAW_NOTIFICATION(false, false);
    }
}

// --- Car Delivery Logic ---
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
    s_xpToNext = 800;
    s_recentRPGain = 0;
    s_recentRPGainTime = 0;
    s_inVehicleLastFrame = false;
    s_lastWantedLevel = 0;
    s_lastVehicle = 0;
    s_deliveredVehicles.clear();
    s_lastDeliveryTime = 0; // Initialize cooldown timer

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
