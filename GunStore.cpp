#include "GunStore.h"
#include "Money.h"
#include "RankBar.h"
#include "RpEvents.h" // <-- ADDED this include
#include "input.h"
#include "script.h"
#include <vector>
#include <fstream>
#include <string>

// --- Global Variables ---
static std::vector<WeaponCategory> g_weaponCategories;
static std::vector<Hash> g_unlockedWeapons;

static int gunCategoryIndex = 0;
static int weaponIndex = 0;
static bool inWeaponSelection = false;

// --- Weapon Locker System ---
bool GunStore_HasWeapon(Hash weaponHash) {
    for (size_t i = 0; i < g_unlockedWeapons.size(); ++i) {
        if (g_unlockedWeapons[i] == weaponHash) {
            return true;
        }
    }
    return false;
}

void GunStore_AddWeapon(Hash weaponHash) {
    if (!GunStore_HasWeapon(weaponHash)) {
        g_unlockedWeapons.push_back(weaponHash);
        GunStore_Save();
    }
}

// New function to clear all bought weapons
void GunStore_ClearAllBoughtWeapons() {
    g_unlockedWeapons.clear(); // Clear the list of unlocked weapons
    GunStore_Save(); // Save the empty list to file
}

void GunStore_Save() {
    std::ofstream file("GTAOfflineGunLocker.ini");
    if (file.is_open()) {
        for (const auto& hash : g_unlockedWeapons) {
            file << hash << std::endl;
        }
        file.close();
    }
}

void GunStore_Load() {
    g_unlockedWeapons.clear();
    std::ifstream file("GTAOfflineGunLocker.ini");
    if (file.is_open()) {
        Hash hash;
        while (file >> hash) {
            g_unlockedWeapons.push_back(hash);
        }
        file.close();
    }
}

// --- Initialization ---
void GunStore_Init() {
    // Pistols
    WeaponCategory pistols;
    pistols.name = "Pistols";
    pistols.weapons.push_back({ "Pistol", GAMEPLAY::GET_HASH_KEY("WEAPON_PISTOL"), 800, 1 });
    pistols.weapons.push_back({ "Pistol .50", GAMEPLAY::GET_HASH_KEY("WEAPON_PISTOL50"), 3900, 1 });
    pistols.weapons.push_back({ "Combat Pistol", GAMEPLAY::GET_HASH_KEY("WEAPON_COMBATPISTOL"), 3200, 9 });
    pistols.weapons.push_back({ "AP Pistol", GAMEPLAY::GET_HASH_KEY("WEAPON_APPISTOL"), 5000, 33 });
    pistols.weapons.push_back({ "SNS Pistol", GAMEPLAY::GET_HASH_KEY("WEAPON_SNSPISTOL"), 2750, 1 }); // lock behind requirements in future?
    pistols.weapons.push_back({ "Heavy Pistol", GAMEPLAY::GET_HASH_KEY("WEAPON_HEAVYPISTOL"), 3750, 1 }); // ---
    g_weaponCategories.push_back(pistols);

    // SMGs
    WeaponCategory smgs;
    smgs.name = "Machine Guns";
    smgs.weapons.push_back({ "Micro SMG", GAMEPLAY::GET_HASH_KEY("WEAPON_MICROSMG"), 3750, 5 });
    smgs.weapons.push_back({ "SMG", GAMEPLAY::GET_HASH_KEY("WEAPON_SMG"), 7500, 11 });
    smgs.weapons.push_back({ "Assault SMG", GAMEPLAY::GET_HASH_KEY("WEAPON_ASSAULTSMG"), 12550, 29 });
    smgs.weapons.push_back({ "MG", GAMEPLAY::GET_HASH_KEY("WEAPON_MG"), 13500, 50 });
    smgs.weapons.push_back({ "Combat MG", GAMEPLAY::GET_HASH_KEY("WEAPON_COMBATMG"), 14800, 80 });
    smgs.weapons.push_back({ "Combat PDW", GAMEPLAY::GET_HASH_KEY("WEAPON_COMBATPDW"), 15000, 1 }); // ---
    g_weaponCategories.push_back(smgs);

    // Rifles
    WeaponCategory rifles;
    rifles.name = "Assault Rifles";
    rifles.weapons.push_back({ "Assault Rifle", GAMEPLAY::GET_HASH_KEY("WEAPON_ASSAULTRIFLE"), 8550, 24 });
    rifles.weapons.push_back({ "Carbine Rifle", GAMEPLAY::GET_HASH_KEY("WEAPON_CARBINERIFLE"), 13000, 42 });
    rifles.weapons.push_back({ "Advanced Rifle", GAMEPLAY::GET_HASH_KEY("WEAPON_ADVANCEDRIFLE"), 14250, 70 });
    rifles.weapons.push_back({ "Special Carbine", GAMEPLAY::GET_HASH_KEY("WEAPON_SPECIALCARBINE"), 14750, 1 }); // ---
    rifles.weapons.push_back({ "Bullpup Rifle", GAMEPLAY::GET_HASH_KEY("WEAPON_BULLPUPRIFLE"), 14500, 1 }); // ---
    g_weaponCategories.push_back(rifles);

    // Shotguns
    WeaponCategory shotguns;
    shotguns.name = "Shotguns";
    shotguns.weapons.push_back({ "Pump Shotgun", GAMEPLAY::GET_HASH_KEY("WEAPON_PUMPSHOTGUN"), 3500, 17 });
    shotguns.weapons.push_back({ "Sawed-Off Shotgun", GAMEPLAY::GET_HASH_KEY("WEAPON_SAWNOFFSHOTGUN"), 2000, 1 }); // --- lock in future and then make it free?
    shotguns.weapons.push_back({ "Assault Shotgun", GAMEPLAY::GET_HASH_KEY("WEAPON_ASSAULTSHOTGUN"), 10000, 37 });
    shotguns.weapons.push_back({ "Heavy Shotgun", GAMEPLAY::GET_HASH_KEY("WEAPON_HEAVYSHOTGUN"), 13550, 1 }); // --
    g_weaponCategories.push_back(shotguns);

    // Sniper Rifles
    WeaponCategory snipers;
    snipers.name = "Sniper Rifles";
    snipers.weapons.push_back({ "Sniper Rifle", GAMEPLAY::GET_HASH_KEY("WEAPON_SNIPERRIFLE"), 20000, 21 });
    snipers.weapons.push_back({ "Heavy Sniper", GAMEPLAY::GET_HASH_KEY("WEAPON_HEAVYSNIPER"), 38150, 90 });
    g_weaponCategories.push_back(snipers);

    // Heavy Weapons
    WeaponCategory heavy;
    heavy.name = "Heavy Weapons";
    heavy.weapons.push_back({ "Grenade Launcher", GAMEPLAY::GET_HASH_KEY("WEAPON_GRENADELAUNCHER"), 32400, 60 });
    heavy.weapons.push_back({ "RPG", GAMEPLAY::GET_HASH_KEY("WEAPON_RPG"), 26250, 100 });
    heavy.weapons.push_back({ "Minigun", GAMEPLAY::GET_HASH_KEY("WEAPON_MINIGUN"), 47000, 120 });
    g_weaponCategories.push_back(heavy);

    // Thrown Weapons
    WeaponCategory thrown;
    thrown.name = "Thrown";
    thrown.weapons.push_back({ "Grenade", GAMEPLAY::GET_HASH_KEY("WEAPON_GRENADE"), 250, 15 });
    thrown.weapons.push_back({ "Sticky Bomb", GAMEPLAY::GET_HASH_KEY("WEAPON_STICKYBOMB"), 600, 19 });
    thrown.weapons.push_back({ "Molotov", GAMEPLAY::GET_HASH_KEY("WEAPON_MOLOTOV"), 200, 31 });
    thrown.weapons.push_back({ "Proximity Mine", GAMEPLAY::GET_HASH_KEY("WEAPON_PROXMINE"), 1000, 1 }); // --
    g_weaponCategories.push_back(thrown);

    // Melee
    WeaponCategory melee;
    melee.name = "Melee";
    melee.weapons.push_back({ "Knife", GAMEPLAY::GET_HASH_KEY("WEAPON_KNIFE"), 400, 1 });
    melee.weapons.push_back({ "Hammer", GAMEPLAY::GET_HASH_KEY("WEAPON_HAMMER"), 500, 1 });
    melee.weapons.push_back({ "Nightstick", GAMEPLAY::GET_HASH_KEY("WEAPON_NIGHTSTICK"), 400, 3 });
    melee.weapons.push_back({ "Bat", GAMEPLAY::GET_HASH_KEY("WEAPON_BAT"), 300, 1 }); // --- and make free
    melee.weapons.push_back({ "Crowbar", GAMEPLAY::GET_HASH_KEY("WEAPON_CROWBAR"), 400, 1 }); // --- and make free
    g_weaponCategories.push_back(melee);

    GunStore_Load();
}

void GunStore_Tick() {
    Ped playerPed = PLAYER::PLAYER_PED_ID();
    for (const auto& weaponHash : g_unlockedWeapons) {
        // Only give weapon if the player doesn't already have it
        // This prevents endlessly giving ammo if the player already has the weapon
        if (!WEAPON::HAS_PED_GOT_WEAPON(playerPed, weaponHash, false)) {
            // Give weapon with max ammo and select it
            WEAPON::GIVE_WEAPON_TO_PED(playerPed, weaponHash, 9999, false, true);
        }
    }
}

void draw_gun_store_menu() {
    extern int menuCategory;
    extern int menuIndex;
    extern int inputDelayFrames;

    const float MENU_X = 0.02f;
    const float MENU_Y = 0.13f;
    const float MENU_W = 0.29f;
    const float MENU_H = 0.038f;

    // Display Player's Money at the top
    char moneyBuf[64];
    sprintf_s(moneyBuf, "Your Money: $%d", Money_Get());
    UI::SET_TEXT_FONT(0);
    UI::SET_TEXT_SCALE(0.0f, 0.37f);
    UI::SET_TEXT_COLOUR(130, 255, 130, 220);
    UI::_SET_TEXT_ENTRY("STRING");
    UI::_ADD_TEXT_COMPONENT_STRING(moneyBuf);
    UI::_DRAW_TEXT(MENU_X, MENU_Y - 0.08f);

    if (!inWeaponSelection) {
        // --- Category Selection ---
        const int numOptions = g_weaponCategories.size() + 1;
        float totalHeight = MENU_H * (numOptions + 1);

        GRAPHICS::DRAW_RECT(MENU_X + MENU_W * 0.5f, MENU_Y - MENU_H * 0.5f + totalHeight * 0.5f, MENU_W, totalHeight, BG_COLOR.r, BG_COLOR.g, BG_COLOR.b, BG_COLOR.a);
        DrawMenuHeader("Gun Store", MENU_X, MENU_Y, MENU_W);

        float optionY = MENU_Y + MENU_H;
        for (size_t i = 0; i < g_weaponCategories.size(); ++i) {
            DrawMenuOption(g_weaponCategories[i].name, MENU_X, optionY + MENU_H * i, MENU_W, MENU_H, i == gunCategoryIndex);
        }
        DrawMenuOption("Back", MENU_X, optionY + MENU_H * g_weaponCategories.size(), MENU_W, MENU_H, g_weaponCategories.size() == gunCategoryIndex);

        // Navigation and activation
        if (inputDelayFrames == 0) { // Only process input if no global input delay
            int up = 0, down = 0;
            if (IsKeyJustUp(VK_NUMPAD8) || PadPressed(DPAD_UP))   up = 1;
            if (IsKeyJustUp(VK_NUMPAD2) || PadPressed(DPAD_DOWN)) down = 1;

            if (up) {
                gunCategoryIndex = (gunCategoryIndex - 1 + numOptions) % numOptions;
                inputDelayFrames = 10; // Apply delay after navigation
            }
            if (down) {
                gunCategoryIndex = (gunCategoryIndex + 1) % numOptions;
                inputDelayFrames = 10; // Apply delay after navigation
            }

            static bool prevA_category = false; // Separate static variable for category A button
            bool currA_category = PadPressed(BTN_A);
            if ((IsKeyJustUp(VK_NUMPAD5) || (currA_category && !prevA_category))) {
                if (gunCategoryIndex == g_weaponCategories.size()) {
                    menuCategory = 0; // CAT_MAIN
                    menuIndex = 6; // Index for Gun Store in main menu
                    inputDelayFrames = 10; // Apply delay after action
                }
                else {
                    inWeaponSelection = true;
                    weaponIndex = 0;
                    inputDelayFrames = 10; // Apply delay after action
                }
            }
            prevA_category = currA_category;
        }
    }
    else {
        // --- Weapon Selection ---
        WeaponCategory& selectedCategory = g_weaponCategories[gunCategoryIndex];
        const int numOptions = selectedCategory.weapons.size() + 1;
        float totalHeight = MENU_H * (numOptions + 1);

        GRAPHICS::DRAW_RECT(MENU_X + MENU_W * 0.5f, MENU_Y - MENU_H * 0.5f + totalHeight * 0.5f, MENU_W, totalHeight, BG_COLOR.r, BG_COLOR.g, BG_COLOR.b, BG_COLOR.a);
        DrawMenuHeader(selectedCategory.name, MENU_X, MENU_Y, MENU_W);

        float optionY = MENU_Y + MENU_H;
        for (size_t i = 0; i < selectedCategory.weapons.size(); ++i) {
            WeaponForSale& gun = selectedCategory.weapons[i];
            bool owned = GunStore_HasWeapon(gun.hash);
            bool unlocked = RpEvents_GetLevel() >= gun.rankRequired;
            char label[128], value[32] = "";

            if (owned) {
                sprintf_s(label, "%s", gun.name);
                sprintf_s(value, "[OWNED]");
            }
            else if (!unlocked) {
                sprintf_s(label, "%s", gun.name);
                sprintf_s(value, "[LOCKED - LVL %d]", gun.rankRequired);
            }
            else {
                sprintf_s(label, "%s", gun.name);
                sprintf_s(value, "$%d", gun.price);
            }
            DrawPairedMenuOption(label, value, MENU_X, optionY + MENU_H * i, MENU_W, MENU_H, i == weaponIndex);
        }
        DrawMenuOption("Back", MENU_X, optionY + MENU_H * selectedCategory.weapons.size(), MENU_W, MENU_H, selectedCategory.weapons.size() == weaponIndex);

        // Navigation and activation
        if (inputDelayFrames == 0) { // Only process input if no global input delay
            int up = 0, down = 0;
            if (IsKeyJustUp(VK_NUMPAD8) || PadPressed(DPAD_UP))   up = 1;
            if (IsKeyJustUp(VK_NUMPAD2) || PadPressed(DPAD_DOWN)) down = 1;

            if (up) {
                weaponIndex = (weaponIndex - 1 + numOptions) % numOptions;
                inputDelayFrames = 10; // Apply delay after navigation
            }
            if (down) {
                weaponIndex = (weaponIndex + 1) % numOptions;
                inputDelayFrames = 10; // Apply delay after navigation
            }

            static bool prevA_weapon = false; // Separate static variable for weapon A button
            bool currA_weapon = PadPressed(BTN_A);
            if ((IsKeyJustUp(VK_NUMPAD5) || (currA_weapon && !prevA_weapon))) {
                if (weaponIndex == selectedCategory.weapons.size()) {
                    inWeaponSelection = false;
                    inputDelayFrames = 10; // Apply delay after action
                }
                else {
                    WeaponForSale& gun = selectedCategory.weapons[weaponIndex];
                    if (GunStore_HasWeapon(gun.hash)) {
                        // Already owned, do nothing
                        UI::_SET_NOTIFICATION_TEXT_ENTRY("STRING");
                        UI::_ADD_TEXT_COMPONENT_STRING("~y~Weapon already owned.");
                        UI::_DRAW_NOTIFICATION(false, true);
                        inputDelayFrames = 10; // Apply delay after action
                    }
                    else if (RpEvents_GetLevel() < gun.rankRequired) {
                        UI::_SET_NOTIFICATION_TEXT_ENTRY("STRING");
                        UI::_ADD_TEXT_COMPONENT_STRING("~r~Rank not high enough!");
                        UI::_DRAW_NOTIFICATION(false, true);
                        inputDelayFrames = 10; // Apply delay after action
                    }
                    else if (Money_Get() >= gun.price) {
                        Money_Add(-gun.price);
                        GunStore_AddWeapon(gun.hash);
                        UI::_SET_NOTIFICATION_TEXT_ENTRY("STRING");
                        UI::_ADD_TEXT_COMPONENT_STRING("~g~Purchase successful!");
                        UI::_DRAW_NOTIFICATION(false, true);
                        // Give weapon to player immediately after purchase
                        WEAPON::GIVE_WEAPON_TO_PED(PLAYER::PLAYER_PED_ID(), gun.hash, 9999, false, true);
                        inputDelayFrames = 10; // Apply delay after action
                    }
                    else {
                        UI::_SET_NOTIFICATION_TEXT_ENTRY("STRING");
                        UI::_ADD_TEXT_COMPONENT_STRING("~r~Not enough money!");
                        UI::_DRAW_NOTIFICATION(false, true);
                        inputDelayFrames = 10; // Apply delay after action
                    }
                }
            }
            prevA_weapon = currA_weapon;
        }
    }
    // Decrement input delay frames at the end of the tick
    if (inputDelayFrames > 0) inputDelayFrames--;
}
