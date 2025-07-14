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

// --- XP/Rank State ---
static int s_playerXP = 0, s_playerLevel = 1, s_xpToNext = 100;
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

// ----- Core XP Reward logic -----
void RpEvents_Reward(int amount, const char* msg)
{
    s_playerXP += amount;
    s_recentRPGain = amount;
    s_recentRPGainTime = GetTickCount64();

    while (s_playerXP >= s_xpToNext) {
        s_playerXP -= s_xpToNext;
        s_playerLevel++;
        s_xpToNext += 50;
        UI::_SET_NOTIFICATION_TEXT_ENTRY("STRING");
        UI::_ADD_TEXT_COMPONENT_STRING("~y~LEVEL UP!");
        UI::_DRAW_NOTIFICATION(false, false);
    }
    if (msg) {
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
    s_xpToNext = 100;
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