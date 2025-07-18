#include "GunStore.h"
#include "Money.h"
#include "RankBar.h"
#include "RpEvents.h" // ADDED this include
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
    // Clear existing categories to rebuild from the provided list
    g_weaponCategories.clear();

    // Pistols
    WeaponCategory pistols;
    pistols.name = "Pistols";
    pistols.weapons.push_back({ "Pistol", GAMEPLAY::GET_HASH_KEY("WEAPON_PISTOL"), 600, 1 });
    pistols.weapons.push_back({ "Pistol Mk II", GAMEPLAY::GET_HASH_KEY("WEAPON_PISTOL_MK2"), 10000, 35 }); // Adjusted from 40
    pistols.weapons.push_back({ "Combat Pistol", GAMEPLAY::GET_HASH_KEY("WEAPON_COMBATPISTOL"), 1200, 5 });
    pistols.weapons.push_back({ "AP Pistol", GAMEPLAY::GET_HASH_KEY("WEAPON_APPISTOL"), 5000, 15 });
    pistols.weapons.push_back({ "Pistol .50", GAMEPLAY::GET_HASH_KEY("WEAPON_PISTOL50"), 7500, 22 });
    pistols.weapons.push_back({ "SNS Pistol", GAMEPLAY::GET_HASH_KEY("WEAPON_SNSPISTOL"), 1000, 3 });
    pistols.weapons.push_back({ "SNS Pistol Mk II", GAMEPLAY::GET_HASH_KEY("WEAPON_SNSPISTOL_MK2"), 8000, 30 }); // Adjusted from 38
    pistols.weapons.push_back({ "Heavy Pistol", GAMEPLAY::GET_HASH_KEY("WEAPON_HEAVYPISTOL"), 3500, 10 });
    pistols.weapons.push_back({ "Vintage Pistol", GAMEPLAY::GET_HASH_KEY("WEAPON_VINTAGEPISTOL"), 4500, 15 });
    pistols.weapons.push_back({ "Marksman Pistol", GAMEPLAY::GET_HASH_KEY("WEAPON_MARKSMANPISTOL"), 6000, 18 });
    pistols.weapons.push_back({ "Revolver", GAMEPLAY::GET_HASH_KEY("WEAPON_REVOLVER"), 7000, 22 });
    pistols.weapons.push_back({ "Revolver Mk II", GAMEPLAY::GET_HASH_KEY("WEAPON_REVOLVER_MK2"), 12000, 50 }); // Adjusted from 55
    pistols.weapons.push_back({ "Flare Gun", GAMEPLAY::GET_HASH_KEY("WEAPON_FLAREGUN"), 1500, 3 });
    pistols.weapons.push_back({ "Stun Gun", GAMEPLAY::GET_HASH_KEY("WEAPON_STUNGUN"), 2000, 8 });
    pistols.weapons.push_back({ "Ceramic Pistol", GAMEPLAY::GET_HASH_KEY("WEAPON_CERAMICPISTOL"), 7000, 28 });
    pistols.weapons.push_back({ "Navy Revolver", GAMEPLAY::GET_HASH_KEY("WEAPON_NAVYREVOLVER"), 9500, 32 });
    pistols.weapons.push_back({ "Double Action Revolver", GAMEPLAY::GET_HASH_KEY("WEAPON_DOUBLEACTION"), 8000, 27 });
    g_weaponCategories.push_back(pistols);

    // Machine Guns (Combined SMGs and LMGs)
    WeaponCategory machineGuns;
    machineGuns.name = "Machine Guns";
    machineGuns.weapons.push_back({ "Micro SMG", GAMEPLAY::GET_HASH_KEY("WEAPON_MICROSMG"), 2500, 8 });
    machineGuns.weapons.push_back({ "SMG", GAMEPLAY::GET_HASH_KEY("WEAPON_SMG"), 7500, 12 });
    machineGuns.weapons.push_back({ "SMG Mk II", GAMEPLAY::GET_HASH_KEY("WEAPON_SMG_MK2"), 15000, 55 }); // Adjusted from 60
    machineGuns.weapons.push_back({ "Assault SMG", GAMEPLAY::GET_HASH_KEY("WEAPON_ASSAULTSMG"), 12500, 20 });
    machineGuns.weapons.push_back({ "Combat PDW", GAMEPLAY::GET_HASH_KEY("WEAPON_COMBATPDW"), 15000, 28 });
    machineGuns.weapons.push_back({ "Machine Pistol", GAMEPLAY::GET_HASH_KEY("WEAPON_MACHINEPISTOL"), 6000, 16 });
    machineGuns.weapons.push_back({ "Mini SMG", GAMEPLAY::GET_HASH_KEY("WEAPON_MINISMG"), 8000, 20 });
    machineGuns.weapons.push_back({ "MG", GAMEPLAY::GET_HASH_KEY("WEAPON_MG"), 14000, 30 });
    machineGuns.weapons.push_back({ "Combat MG", GAMEPLAY::GET_HASH_KEY("WEAPON_COMBATMG"), 18000, 40 });
    machineGuns.weapons.push_back({ "Combat MG Mk II", GAMEPLAY::GET_HASH_KEY("WEAPON_COMBATMG_MK2"), 22000, 80 }); // Adjusted from 90
    machineGuns.weapons.push_back({ "Gusenberg Sweeper", GAMEPLAY::GET_HASH_KEY("WEAPON_GUSENBERG"), 16000, 65 });
    g_weaponCategories.push_back(machineGuns);

    // Assault Rifles
    WeaponCategory assaultRifles;
    assaultRifles.name = "Assault Rifles";
    assaultRifles.weapons.push_back({ "Assault Rifle", GAMEPLAY::GET_HASH_KEY("WEAPON_ASSAULTRIFLE"), 10000, 18 });
    assaultRifles.weapons.push_back({ "Assault Rifle Mk II", GAMEPLAY::GET_HASH_KEY("WEAPON_ASSAULTRIFLE_MK2"), 18000, 85 });
    assaultRifles.weapons.push_back({ "Carbine Rifle", GAMEPLAY::GET_HASH_KEY("WEAPON_CARBINERIFLE"), 15000, 25 });
    assaultRifles.weapons.push_back({ "Carbine Rifle Mk II", GAMEPLAY::GET_HASH_KEY("WEAPON_CARBINERIFLE_MK2"), 20000, 95 });
    assaultRifles.weapons.push_back({ "Advanced Rifle", GAMEPLAY::GET_HASH_KEY("WEAPON_ADVANCEDRIFLE"), 22000, 35 });
    assaultRifles.weapons.push_back({ "Special Carbine", GAMEPLAY::GET_HASH_KEY("WEAPON_SPECIALCARBINE"), 18000, 26 });
    assaultRifles.weapons.push_back({ "Special Carbine Mk II", GAMEPLAY::GET_HASH_KEY("WEAPON_SPECIALCARBINE_MK2"), 22000, 90 }); // Adjusted from 99
    assaultRifles.weapons.push_back({ "Bullpup Rifle", GAMEPLAY::GET_HASH_KEY("WEAPON_BULLPUPRIFLE"), 16500, 24 });
    assaultRifles.weapons.push_back({ "Bullpup Rifle Mk II", GAMEPLAY::GET_HASH_KEY("WEAPON_BULLPUPRIFLE_MK2"), 21000, 90 });
    assaultRifles.weapons.push_back({ "Compact Rifle", GAMEPLAY::GET_HASH_KEY("WEAPON_COMPACTRIFLE"), 10000, 28 });
    assaultRifles.weapons.push_back({ "Military Rifle", GAMEPLAY::GET_HASH_KEY("WEAPON_MILITARYRIFLE"), 17000, 80 });
    assaultRifles.weapons.push_back({ "Heavy Rifle", GAMEPLAY::GET_HASH_KEY("WEAPON_HEAVYRIFLE"), 19000, 88 });
    g_weaponCategories.push_back(assaultRifles);

    // Shotguns
    WeaponCategory shotguns;
    shotguns.name = "Shotguns";
    shotguns.weapons.push_back({ "Pump Shotgun", GAMEPLAY::GET_HASH_KEY("WEAPON_PUMPSHOTGUN"), 3500, 10 });
    shotguns.weapons.push_back({ "Pump Shotgun Mk II", GAMEPLAY::GET_HASH_KEY("WEAPON_PUMPSHOTGUN_MK2"), 15000, 75 });
    shotguns.weapons.push_back({ "Sawed-Off Shotgun", GAMEPLAY::GET_HASH_KEY("WEAPON_SAWNOFFSHOTGUN"), 2000, 6 });
    shotguns.weapons.push_back({ "Assault Shotgun", GAMEPLAY::GET_HASH_KEY("WEAPON_ASSAULTSHOTGUN"), 10000, 30 });
    shotguns.weapons.push_back({ "Heavy Shotgun", GAMEPLAY::GET_HASH_KEY("WEAPON_HEAVYSHOTGUN"), 12000, 32 });
    shotguns.weapons.push_back({ "Bullpup Shotgun", GAMEPLAY::GET_HASH_KEY("WEAPON_BULLPUPSHOTGUN"), 11000, 30 });
    shotguns.weapons.push_back({ "Musket", GAMEPLAY::GET_HASH_KEY("WEAPON_MUSKET"), 9000, 28 });
    shotguns.weapons.push_back({ "Double Barrel Shotgun", GAMEPLAY::GET_HASH_KEY("WEAPON_DBSHOTGUN"), 8500, 26 });
    shotguns.weapons.push_back({ "Sweeper Shotgun", GAMEPLAY::GET_HASH_KEY("WEAPON_AUTOSHOTGUN"), 14000, 35 });
    shotguns.weapons.push_back({ "Combat Shotgun", GAMEPLAY::GET_HASH_KEY("WEAPON_COMBATSHOTGUN"), 12000, 40 });
    g_weaponCategories.push_back(shotguns);

    // Sniper Rifles
    WeaponCategory snipers;
    snipers.name = "Sniper Rifles";
    snipers.weapons.push_back({ "Sniper Rifle", GAMEPLAY::GET_HASH_KEY("WEAPON_SNIPERRIFLE"), 20000, 21 });
    snipers.weapons.push_back({ "Heavy Sniper", GAMEPLAY::GET_HASH_KEY("WEAPON_HEAVYSNIPER"), 38000, 50 });
    snipers.weapons.push_back({ "Heavy Sniper Mk II", GAMEPLAY::GET_HASH_KEY("WEAPON_HEAVYSNIPER_MK2"), 45000, 90 });
    snipers.weapons.push_back({ "Marksman Rifle", GAMEPLAY::GET_HASH_KEY("WEAPON_MARKSMANRIFLE"), 22000, 45 });
    snipers.weapons.push_back({ "Marksman Rifle Mk II", GAMEPLAY::GET_HASH_KEY("WEAPON_MARKSMANRIFLE_MK2"), 28000, 80 });
    snipers.weapons.push_back({ "Precision Rifle", GAMEPLAY::GET_HASH_KEY("WEAPON_PRECISIONRIFLE"), 25000, 55 });
    g_weaponCategories.push_back(snipers);

    // Heavy Weapons
    WeaponCategory heavy;
    heavy.name = "Heavy Weapons";
    heavy.weapons.push_back({ "Grenade Launcher", GAMEPLAY::GET_HASH_KEY("WEAPON_GRENADELAUNCHER"), 35000, 45 });
    heavy.weapons.push_back({ "RPG", GAMEPLAY::GET_HASH_KEY("WEAPON_RPG"), 75000, 60 });
    heavy.weapons.push_back({ "Minigun", GAMEPLAY::GET_HASH_KEY("WEAPON_MINIGUN"), 50000, 70 });
    heavy.weapons.push_back({ "Homing Launcher", GAMEPLAY::GET_HASH_KEY("WEAPON_HOMINGLAUNCHER"), 50000, 95 });
    heavy.weapons.push_back({ "Firework Launcher", GAMEPLAY::GET_HASH_KEY("WEAPON_FIREWORK"), 10000, 1 });
    heavy.weapons.push_back({ "Railgun", GAMEPLAY::GET_HASH_KEY("WEAPON_RAILGUN"), 60000, 99 });
    heavy.weapons.push_back({ "Unholy Hellbringer", GAMEPLAY::GET_HASH_KEY("WEAPON_RAYCARBINE"), 85000, 85 }); // Adjusted from 99
    heavy.weapons.push_back({ "Widowmaker", GAMEPLAY::GET_HASH_KEY("WEAPON_RAYMINIGUN"), 80000, 85 }); // Adjusted from 99
    g_weaponCategories.push_back(heavy);

    // Thrown Weapons
    WeaponCategory thrown;
    thrown.name = "Thrown";
    thrown.weapons.push_back({ "Grenade", GAMEPLAY::GET_HASH_KEY("WEAPON_GRENADE"), 250, 5 });
    thrown.weapons.push_back({ "Sticky Bomb", GAMEPLAY::GET_HASH_KEY("WEAPON_STICKYBOMB"), 600, 12 });
    thrown.weapons.push_back({ "Proximity Mine", GAMEPLAY::GET_HASH_KEY("WEAPON_PROXMINE"), 1000, 18 });
    thrown.weapons.push_back({ "Molotov", GAMEPLAY::GET_HASH_KEY("WEAPON_MOLOTOV"), 500, 10 });
    thrown.weapons.push_back({ "Tear Gas", GAMEPLAY::GET_HASH_KEY("WEAPON_SMOKEGRENADE"), 300, 10 });
    thrown.weapons.push_back({ "Pipe Bomb", GAMEPLAY::GET_HASH_KEY("WEAPON_PIPEBOMB"), 400, 12 });
    thrown.weapons.push_back({ "Snowball", GAMEPLAY::GET_HASH_KEY("WEAPON_SNOWBALL"), 0, 1 });
    thrown.weapons.push_back({ "Ball", GAMEPLAY::GET_HASH_KEY("WEAPON_BALL"), 0, 1 });
    thrown.weapons.push_back({ "BZ Gas", GAMEPLAY::GET_HASH_KEY("WEAPON_BZGAS"), 450, 15 });
    thrown.weapons.push_back({ "Petrol Can", GAMEPLAY::GET_HASH_KEY("WEAPON_PETROLCAN"), 50, 1 });
    g_weaponCategories.push_back(thrown);

    // Melee
    WeaponCategory melee_new; // Renamed to avoid conflict with initial 'melee' variable
    melee_new.name = "Melee";
    melee_new.weapons.push_back({ "Knife", GAMEPLAY::GET_HASH_KEY("WEAPON_KNIFE"), 200, 1 });
    melee_new.weapons.push_back({ "Nightstick", GAMEPLAY::GET_HASH_KEY("WEAPON_NIGHTSTICK"), 100, 1 });
    melee_new.weapons.push_back({ "Hammer", GAMEPLAY::GET_HASH_KEY("WEAPON_HAMMER"), 500, 1 });
    melee_new.weapons.push_back({ "Bat", GAMEPLAY::GET_HASH_KEY("WEAPON_BAT"), 300, 1 });
    melee_new.weapons.push_back({ "Crowbar", GAMEPLAY::GET_HASH_KEY("WEAPON_CROWBAR"), 400, 1 });
    // Add other melee weapons that were in the previous list but not in the user's new short list for Melee
    melee_new.weapons.push_back({ "Golf Club", GAMEPLAY::GET_HASH_KEY("WEAPON_GOLFCLUB"), 250, 4 });
    melee_new.weapons.push_back({ "Antique Cavalry Dagger", GAMEPLAY::GET_HASH_KEY("WEAPON_DAGGER"), 600, 6 }); // Specific request
    melee_new.weapons.push_back({ "Hatchet", GAMEPLAY::GET_HASH_KEY("WEAPON_HATCHET"), 700, 7 });
    melee_new.weapons.push_back({ "Knuckle Duster", GAMEPLAY::GET_HASH_KEY("WEAPON_KNUCKLE"), 800, 9 });
    melee_new.weapons.push_back({ "Machete", GAMEPLAY::GET_HASH_KEY("WEAPON_MACHETE"), 900, 11 });
    melee_new.weapons.push_back({ "Switchblade", GAMEPLAY::GET_HASH_KEY("WEAPON_SWITCHBLADE"), 750, 10 });
    melee_new.weapons.push_back({ "Battle Axe", GAMEPLAY::GET_HASH_KEY("WEAPON_BATTLEAXE"), 1200, 14 });
    melee_new.weapons.push_back({ "Pool Cue", GAMEPLAY::GET_HASH_KEY("WEAPON_POOLCUE"), 200, 1 });
    melee_new.weapons.push_back({ "Wrench", GAMEPLAY::GET_HASH_KEY("WEAPON_WRENCH"), 350, 5 });
    melee_new.weapons.push_back({ "Stone Hatchet", GAMEPLAY::GET_HASH_KEY("WEAPON_STONE_HATCHET"), 1500, 18 });
    melee_new.weapons.push_back({ "Flashlight", GAMEPLAY::GET_HASH_KEY("WEAPON_FLASHLIGHT"), 100, 1 });
    melee_new.weapons.push_back({ "Broken Bottle", GAMEPLAY::GET_HASH_KEY("WEAPON_BOTTLE"), 150, 1 });
    g_weaponCategories.push_back(melee_new);

    // Gadgets
    WeaponCategory gadgets;
    gadgets.name = "Gadgets";
    gadgets.weapons.push_back({ "Parachute", GAMEPLAY::GET_HASH_KEY("GADGET_PARACHUTE"), 500, 1 });
    g_weaponCategories.push_back(gadgets);

    GunStore_Load();
}

void GunStore_Tick() {
    Ped playerPed = PLAYER::PLAYER_PED_ID();
    for (const auto& weaponHash : g_unlockedWeapons) {
        // Only give weapon if the player doesn't already have it
        // This prevents endlessly giving ammo if the player already has the weapon
        if (!WEAPON::HAS_PED_GOT_WEAPON(playerPed, weaponHash, false)) {
            // Give weapon with max ammo and select it
            WEAPON::GIVE_WEAPON_TO_PED(PLAYER::PLAYER_PED_ID(), weaponHash, 9999, false, true);
        }
    }
}

void draw_gun_store_menu() {
    extern int menuCategory;
    extern int menuIndex;
    extern int inputDelayFrames;

    extern const float MENU_X;
    extern const float MENU_Y;
    extern const float MENU_W;
    extern const float MENU_H;
    extern const RGBA BG_COLOR;
    extern const RGBA HEADER_COLOR;
    extern const RGBA OPTION_COLOR;
    extern const RGBA SELECTED_COLOR;
    extern const RGBA TEXT_COLOR;
    extern const RGBA SELECTED_TEXT_COLOR;
    extern const RGBA HEADER_TEXT_COLOR;
    extern const RGBA TAB_BG_COLOR;
    extern const RGBA SELECTED_TAB_COLOR;
    extern const int FONT;


    // Display Player's Money at the top
    char moneyBuf[64];
    sprintf_s(moneyBuf, "Your Money: $%d", Money_Get());
    UI::SET_TEXT_FONT(0);
    UI::SET_TEXT_SCALE(0.0f, 0.37f);
    UI::SET_TEXT_COLOUR(130, 255, 130, 220);
    UI::_SET_TEXT_ENTRY("STRING");
    UI::_ADD_TEXT_COMPONENT_STRING(moneyBuf);
    UI::_DRAW_TEXT(MENU_X, MENU_Y - 0.08f);

    // Only process input if no global input delay
    // This check is moved up to apply to all input processing in this function.
    bool canProcessInput = (inputDelayFrames == 0);


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
        if (canProcessInput) {
            int up = 0, down = 0;
            // Added arrow key support
            if (IsKeyJustUp(VK_NUMPAD8) || IsKeyJustUp(VK_UP) || PadPressed(DPAD_UP))   up = 1;
            if (IsKeyJustUp(VK_NUMPAD2) || IsKeyJustUp(VK_DOWN) || PadPressed(DPAD_DOWN)) down = 1;

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
            // Added Enter key support
            if ((IsKeyJustUp(VK_NUMPAD5) || IsKeyJustUp(VK_RETURN) || (currA_category && !prevA_category))) {
                if (gunCategoryIndex == g_weaponCategories.size()) { // "Back" option selected
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
            // Allow Back/Escape to go back from category selection
            if (IsKeyJustUp(VK_NUMPAD0) || IsKeyJustUp(VK_BACK) || IsKeyJustUp(VK_ESCAPE) || PadPressed(BTN_B)) {
                menuCategory = 0; // CAT_MAIN
                menuIndex = 6; // Index for Gun Store in main menu
                inputDelayFrames = 10; // Apply delay after action
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
        if (canProcessInput) {
            int up = 0, down = 0;
            // Added arrow key support
            if (IsKeyJustUp(VK_NUMPAD8) || IsKeyJustUp(VK_UP) || PadPressed(DPAD_UP))   up = 1;
            if (IsKeyJustUp(VK_NUMPAD2) || IsKeyJustUp(VK_DOWN) || PadPressed(DPAD_DOWN)) down = 1;

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
            // Added Enter key support
            if ((IsKeyJustUp(VK_NUMPAD5) || IsKeyJustUp(VK_RETURN) || (currA_weapon && !prevA_weapon))) {
                if (weaponIndex == selectedCategory.weapons.size()) { // "Back" option selected
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
            // Allow Back/Escape to go back from weapon selection
            if (IsKeyJustUp(VK_NUMPAD0) || IsKeyJustUp(VK_BACK) || IsKeyJustUp(VK_ESCAPE) || PadPressed(BTN_B)) {
                inWeaponSelection = false;
                inputDelayFrames = 10; // Apply delay after action
            }
            prevA_weapon = currA_weapon;
        }
    }
    // Decrement input delay frames at the end of the tick
    if (inputDelayFrames > 0) inputDelayFrames--;
}