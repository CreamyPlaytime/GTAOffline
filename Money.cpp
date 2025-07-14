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
    char buf[64];
    sprintf(buf, "$%d", g_money);

    float x = 0.88f, y = 0.045f, scale = 0.63f;

    int r = (GetTickCount() - g_lastPickupTime < 1800) ? 32 : 0;
    int g = (GetTickCount() - g_lastPickupTime < 1800) ? 255 : 220;
    int b = (GetTickCount() - g_lastPickupTime < 1800) ? 32 : 220;

    UI::SET_TEXT_FONT(7);
    UI::SET_TEXT_SCALE(0.0f, scale);
    UI::SET_TEXT_COLOUR(r, g, b, 240);
    UI::SET_TEXT_CENTRE(0);
    UI::SET_TEXT_DROPSHADOW(2, 0, 0, 0, 255);
    UI::_SET_TEXT_ENTRY("STRING");
    UI::_ADD_TEXT_COMPONENT_STRING(buf);
    UI::_DRAW_TEXT(x, y);

    if (GetTickCount() - g_lastPickupTime < 1200 && g_justPickedUp > 0) {
        char addbuf[32];
        sprintf(addbuf, "+$%d", g_justPickedUp);
        UI::SET_TEXT_FONT(0);
        UI::SET_TEXT_SCALE(0.0f, 0.38f);
        UI::SET_TEXT_COLOUR(36, 220, 60, 222);
        UI::_SET_TEXT_ENTRY("STRING");
        UI::_ADD_TEXT_COMPONENT_STRING(addbuf);
        UI::_DRAW_TEXT(x + 0.067f, y + 0.037f);
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
void Money_Init() { srand((unsigned int)time(0)); }

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
