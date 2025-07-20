#define _CRT_SECURE_NO_WARNINGS
#include "script.h"
#include "Money.h"
#include "RpEvents.h" // Added for RP rewards
#include <unordered_set>
#include <map>
#include <fstream>
#include <sstream>
#include <cmath>
#include <cstdio>
#include <ctime>
#include <string> // Required for std::string and std::to_string
#include <algorithm> // IMPORTANT: Still included for completeness, but std::max call is removed

// --- Money pickup prop hashes ---
static const Hash moneyBagHash = 0xEE5EBC97; // "prop_money_bag_01"
static const Hash cashCaseHash = 0x113FD533; // "prop_cash_case_01"
static const Hash moneyRollHash = 0x9CA6F755; // "prop_poly_bag_01"

// Array of all pickup types the script will scan for.
static const Hash moneyPickupHashes[] = {
    moneyBagHash,
    cashCaseHash,
    moneyRollHash
};

// --- Global State ---
static int g_money = 0;
static int g_justPickedUp = 0;
static DWORD g_lastPickupTime = 0;
static float s_displayedMoney = 0.0f; // NEW: For smooth animation

// This set stores the handles of pickups we've already collected to prevent double payments.
static std::unordered_set<Object> g_collectedPickups;

// --- Weighted random money calculation for standard bags ---
int GetWeightedRandomMoney() {
    int r = rand() % 1000;
    if (r < 600)       return 10 + rand() % 11;      // $10 - $20 (60%)
    else if (r < 850)  return 21 + rand() % 80;      // $21 - $100 (25%)
    else if (r < 980)  return 101 + rand() % 400;    // $101 - $500 (13%)
    else if (r < 998)  return 501 + rand() % 4500;   // $501 - $5000 (1.8%)
    else               return 5001 + rand() % 15000; // $5001 - $20000 (0.2%)
}

// --- Random money calculation for the high-value cash case ---
int GetRandomBigMoney() {
    // Returns a random amount between $20,000 and $500,000
    return 20000 + (rand() % 480001);
}


// --- Scanning logic for money pickups ---
void Money_PickupScan() {
    Ped playerPed = PLAYER::PLAYER_PED_ID();
    if (!ENTITY::DOES_ENTITY_EXIST(playerPed)) return;

    Vector3 playerPos = ENTITY::GET_ENTITY_COORDS(playerPed, true);
    float collectionRadius = 1.8f;

    for (const Hash& pickupHash : moneyPickupHashes) {
        Object closestObject = OBJECT::GET_CLOSEST_OBJECT_OF_TYPE(
            playerPos.x, playerPos.y, playerPos.z,
            collectionRadius,
            pickupHash,
            false, false, true
        );

        if (ENTITY::DOES_ENTITY_EXIST(closestObject)) {
            if (g_collectedPickups.find(closestObject) == g_collectedPickups.end()) {

                int amount = 0;
                Hash modelHash = ENTITY::GET_ENTITY_MODEL(closestObject);

                if (modelHash == moneyBagHash) {
                    amount = GetWeightedRandomMoney();
                    // No RP for generic NPC money bags
                }
                else if (modelHash == cashCaseHash || modelHash == moneyRollHash) {
                    amount = GetRandomBigMoney();
                    // FIXED: Only give RP for the high-value store bags
                    RpEvents_Reward(50, "~g~RP: Store Robbery");
                }

                if (amount > 0) {
                    Money_Add(amount);
                }

                g_collectedPickups.insert(closestObject);
                ENTITY::SET_ENTITY_AS_MISSION_ENTITY(closestObject, false, true);
                ENTITY::DELETE_ENTITY(&closestObject);
                break;
            }
        }
    }
}


// --- Money Draw (UI) ---
void Money_Draw() {
    // Original positioning for the starting point
    const float x_original = 0.88f; // The fixed starting X for short numbers
    const float y = 0.045f;
    const float scale = 0.63f;

    int r = (GetTickCount() - g_lastPickupTime < 1800) ? 32 : 0;
    int g = (GetTickCount() - g_lastPickupTime < 1800) ? 255 : 220;
    int b = (GetTickCount() - g_lastPickupTime < 1800) ? 32 : 220;

    // NEW: Smoothly interpolate displayed money towards actual money
    const float interpolationFactor = 0.07f; // Adjust for desired animation speed
    s_displayedMoney = s_displayedMoney * (1.0f - interpolationFactor) + (float)g_money * interpolationFactor;

    // Snap to target if very close to prevent jitter from floating-point inaccuracies
    if (fabs(s_displayedMoney - g_money) < 0.5f) {
        s_displayedMoney = (float)g_money;
    }

    // Format the money string
    char buf[64];
    sprintf(buf, "$%d", (int)s_displayedMoney);
    std::string moneyString = buf; // Convert to std::string to use .length()

    // --- Dynamic X adjustment to "scooch" left based on threshold ---
    // Estimate text width per character. Fine-tune this value if necessary.
    const float char_width_estimate = 0.008f;
    // THRESHOLD_CHARS is the maximum string length (including '$') before scooching begins.
    // Setting to 7 means strings with 8 characters (e.g., "$1000000" for 1 million) and above will scooch.
    const int THRESHOLD_CHARS = 7; // Changed to 7 for 7-digit value threshold.

    float current_x = x_original; // Default to the original starting X

    // Only apply dynamic scooching if the string length is greater than the threshold
    if (moneyString.length() > THRESHOLD_CHARS) {
        // Calculate the width of the characters *beyond* the threshold
        // Explicitly cast to float to prevent potential type mismatch errors
        float overshoot_width = (static_cast<float>(moneyString.length()) - THRESHOLD_CHARS) * char_width_estimate * (scale / 0.63f);

        // Shift the starting X to the left by this overshoot amount
        current_x = x_original - overshoot_width;

        // The std::max call to prevent going off-screen to the left has been removed due to compilation issues.
        // If money becomes astronomically large, it might go off the left side of the screen.
        // current_x = std::max(current_x, 0.02f); // Removed this line.
    }

    UI::SET_TEXT_FONT(7);
    UI::SET_TEXT_SCALE(0.0f, scale);
    UI::SET_TEXT_COLOUR(r, g, b, 240);
    UI::SET_TEXT_CENTRE(0); // Keep left alignment for the text
    UI::SET_TEXT_DROPSHADOW(2, 0, 0, 0, 255);
    UI::_SET_TEXT_ENTRY("STRING");
    UI::_ADD_TEXT_COMPONENT_STRING(buf);
    UI::_DRAW_TEXT(current_x, y); // Use dynamically adjusted X

    if (GetTickCount() - g_lastPickupTime < 1200 && g_justPickedUp > 0) {
        char addbuf[32];
        sprintf(addbuf, "+$%d", g_justPickedUp);
        // Position the "+$amount" notification relative to the main money display's dynamic X.
        // It generally stays slightly to the_right of the main money, moving with it.
        // Explicitly cast to float for calculation
        float main_text_actual_width_for_offset = static_cast<float>(moneyString.length()) * char_width_estimate * (scale / 0.63f);
        float add_x = current_x + main_text_actual_width_for_offset + 0.01f; // Offset to the right of the main money string
        float add_y = y + 0.037f;

        UI::SET_TEXT_FONT(0);
        UI::SET_TEXT_SCALE(0.0f, 0.38f);
        UI::SET_TEXT_COLOUR(36, 220, 60, 222);
        UI::SET_TEXT_CENTRE(0); // Keep left alignment for notification
        UI::_SET_TEXT_ENTRY("STRING");
        UI::_ADD_TEXT_COMPONENT_STRING(addbuf);
        UI::_DRAW_TEXT(add_x, add_y);
    }
}

// --- Money API Functions ---
void Money_Add(int amount) {
    g_money += amount;
    g_justPickedUp = amount;
    g_lastPickupTime = GetTickCount();
}
int Money_Get() { return g_money; }
void Money_Set(int value) { g_money = value; }
void Money_Init() {
    srand((unsigned int)time(0));
    s_displayedMoney = (float)g_money; // NEW: Initialize displayed money for animation
}

// --- Persistent Save/Load ---
void Money_Save(const char* iniPath) {
    std::ofstream out(iniPath, std::ios::trunc);
    out << "money=" << g_money << "\n";
    out.close();
}

void Money_Load(const char* iniPath) {
    std::ifstream in(iniPath);
    if (!in) return;
    std::string line;
    while (std::getline(in, line)) {
        if (line.find("money=") == 0) {
            try {
                g_money = std::stoi(line.substr(6));
            }
            catch (...) {
                g_money = 0;
            }
        }
    }
    in.close();
}