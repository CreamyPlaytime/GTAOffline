#define _CRT_SECURE_NO_WARNINGS
#define NOMINMAX
#include "Cheats.h"
#include "Weapons.h"
#include "script.h"
#include "input.h"
#include <cstdio>
#include <cmath>
#include <windows.h>
#include <Xinput.h>
#include <algorithm>
#pragma comment(lib, "Xinput9_1_0.lib")

// --- Weapon Hashes List (All Known GTA V Weapons, Alloc8or/NativeDB/2024) ---
static const Hash weaponHashes[] = {
    0x92A27487, 0x99B507EA, 0x678B81B1, 0x4E875F73, 0x958A4A8F, 0x440E4788, 0x84BD7BFD, 0xF9E6AA4B, 0x8BB05FD7, 0xF9E85D51, 0xAF282D23, 0xD8DF3C3C, 0xCD274149, 0x94117305, 0x3813FC08, 0x19044EE0, 0x88374054, 0xCE580A77,
    0x1B06D571, 0xBFE256D4, 0x5EF9FEC4, 0x22D8FE39, 0x99AEEB3B, 0xBFD21232, 0xD205520E, 0x83839C4, 0x47757124, 0xC1B3C3D1, 0xCB96392F, 0x97EA20B8, 0xAF113F99, 0xDC4DB296, 0x969C3D67, 0x3AABBBAA, 0xAF3696A1, 0x3656C8C1, 0xD1D5F52B, 0x476BF155,
    0x13532244, 0x2BE6766B, 0x78A97CD0, 0xEFE7E2DF, 0xDB1AA450, 0xBD248B55, 0xE284C527,
    0x1D073A89, 0x555AF99A, 0x7846A318, 0x9D61E50F, 0x958A4A8F, 0xEF951FBB, 0xCE6A7B76, 0x9D07F764, 0x5A96BA4,
    0xBFEFFF6D, 0x394F415C, 0x83BF0278, 0xFAD1F1C9, 0xC0A3098D, 0x7F229F94, 0x84D6FAFD, 0x624FE830, 0x969C3D67, 0x7FD62962, 0x3E271019,
    0x05FC3C11, 0x0C472FE2, 0xA914799, 0x6A6C02E0, 0x6C5B941A, 0x63AB0442,
    0xA284510B, 0x4DD2DC56, 0x42BF8A85, 0xB1CA77B1, 0x93E220BD, 0x687652CE, 0x497FACC3, 0xA0973D5E, 0xBA45E8B8, 0x4277DB15, 0xB62D1F67,
    0x24B17070, 0xFDBC8A50, 0x2C3731D9, 0xFBAB5776, 0xBA536372, 0x23C9F95C, 0x060EC506, 0xAB564B93,
    0xC734385A, 0x34A67B97, 0xA2719263
};
static const int numWeapons = sizeof(weaponHashes) / sizeof(weaponHashes[0]);

// ------ Weapons Tab State ------
bool infAmmo = false, explosiveAmmo = false, fireAmmo = false;
bool explosiveMelee = false, rapidFire = false, giveAllWeapons = false;
bool forceGun = false, magnetGun = false;
bool soulSwapGun = false;
float forceMultiplier = 10.0f;
float damageMultiplier = 1.0f;
float magnetGripStrength = 2.0f;
float magnetHoldDistance = 8.0f;
float magnetLaunchPower = 500.0f;
float magnetBoostPower = 400.0f;


// ---- Rapid Fire Logic ----
static DWORD lastRapidFireTick = 0;
static void handleRapidFire(Ped ped) {
    if (!rapidFire) return;
    bool shooting = KeyHeld(VK_LBUTTON) || RT_Held();
    if (!shooting) return;
    DWORD now = GetTickCount();
    if (now - lastRapidFireTick < 18) return;
    lastRapidFireTick = now;
    Vector3 camPos = CAM::GET_GAMEPLAY_CAM_COORD();
    Vector3 camDir;
    {   // Calculate camera forward vector (basic)
        Vector3 rot = CAM::GET_GAMEPLAY_CAM_ROT(2);
        float radPitch = rot.x * 0.0174532924f;
        float radYaw = rot.z * 0.0174532924f;
        camDir.x = -sinf(radYaw) * cosf(radPitch);
        camDir.y = cosf(radYaw) * cosf(radPitch);
        camDir.z = sinf(radPitch);
    }
    float dist = 200.0f;
    Vector3 target;
    target.x = camPos.x + camDir.x * dist;
    target.y = camPos.y + camDir.y * dist;
    target.z = camPos.z + camDir.z * dist;
    Hash curWeap;
    if (!WEAPON::GET_CURRENT_PED_WEAPON(ped, &curWeap, 1)) return;
    WEAPON::SET_PED_AMMO(ped, curWeap, 9999);
    GAMEPLAY::SHOOT_SINGLE_BULLET_BETWEEN_COORDS(
        camPos.x, camPos.y, camPos.z,
        target.x, target.y, target.z,
        50, 1, curWeap, ped, TRUE, TRUE, 200.0f
    );
}

// ---- Give All Weapons Logic ----
static void handleGiveAllWeapons(Ped ped) {
    for (int i = 0; i < numWeapons; ++i)
        WEAPON::GIVE_WEAPON_TO_PED(ped, weaponHashes[i], 9999, false, false);
}
// NEW: Handles the global weapon damage multiplier
void handleDamageMultiplier(Player p)
{
    // This native applies a multiplier to all damage dealt by the player's weapons.
    // It's called every tick to ensure the value from the menu is always applied.
    PLAYER::SET_PLAYER_WEAPON_DAMAGE_MODIFIER(p, damageMultiplier);
}
// ---- Force Gun Logic ----
// ---- Force Gun Logic (Updated) ----
static void HandleForceGun() {
    if (!forceGun) return;
    static DWORD lastShotTick = 0;
    Player player = PLAYER::PLAYER_ID();
    Ped playerPed = PLAYER::PLAYER_PED_ID();
    if (!PED::IS_PED_SHOOTING(playerPed))
        return;
    DWORD now = GetTickCount();
    if (now - lastShotTick < 90) return;
    lastShotTick = now;
    Entity aimedEntity = 0;
    if (!PLAYER::GET_ENTITY_PLAYER_IS_FREE_AIMING_AT(player, &aimedEntity)) return;
    if (!ENTITY::DOES_ENTITY_EXIST(aimedEntity)) return;
    if (ENTITY::IS_ENTITY_A_PED(aimedEntity)) {
        Ped ped = (Ped)aimedEntity;
        if (PED::IS_PED_IN_ANY_VEHICLE(ped, false)) {
            Vehicle veh = PED::GET_VEHICLE_PED_IS_IN(ped, false);
            if (ENTITY::DOES_ENTITY_EXIST(veh))
                aimedEntity = veh;
        }
        else if (!PED::IS_PED_RAGDOLL(ped)) {
            PED::SET_PED_TO_RAGDOLL(ped, 1000, 1000, 0, false, false, false);
        }
    }
    Vector3 camPos = CAM::GET_GAMEPLAY_CAM_COORD();
    Vector3 targetPos = ENTITY::GET_ENTITY_COORDS(aimedEntity, true);
    Vector3 dir;
    dir.x = targetPos.x - camPos.x;
    dir.y = targetPos.y - camPos.y;
    dir.z = targetPos.z - camPos.z;
    float mag = sqrtf(dir.x * dir.x + dir.y * dir.y + dir.z * dir.z);
    if (mag < 0.01f) return;
    dir.x /= mag; dir.y /= mag; dir.z /= mag;

    // Just set a fixed bullet endpoint 50 units in the aim direction (arbitrary, adjust if you want)
    Vector3 bulletEnd = { camPos.x + dir.x * 50.0f, camPos.y + dir.y * 50.0f, camPos.z + dir.z * 50.0f };

    int bulletDmg = 50;
    Hash weaponHash;
    WEAPON::GET_CURRENT_PED_WEAPON(playerPed, &weaponHash, 1);
    GAMEPLAY::SHOOT_SINGLE_BULLET_BETWEEN_COORDS(
        camPos.x, camPos.y, camPos.z,
        bulletEnd.x, bulletEnd.y, bulletEnd.z,
        bulletDmg, 1, weaponHash, playerPed, true, false, 150.0f
    );
    if (forceMultiplier > 0.0f) {
        ENTITY::APPLY_FORCE_TO_ENTITY(
            aimedEntity, 1,
            dir.x * forceMultiplier, dir.y * forceMultiplier, dir.z * forceMultiplier,
            0, 0, 0, 0, false, true, true, false, true
        );
    }
}

// ---- Magnet Gun Logic ----
void HandleMagnetGun()
{
    if (!magnetGun) return;

    Player player = PLAYER::PLAYER_ID();
    Ped playerPed = PLAYER::PLAYER_PED_ID();

    static Entity magnetTarget = 0;
    static bool   holding = false;
    static DWORD  launchTick = 0;
    static Vector3 launchDir = { 0, 0, 0 };

    // --- 1. Only while aiming: "Hold" mode ---
    if (PLAYER::IS_PLAYER_FREE_AIMING(player))
    {
        // Try to pick up an entity if not already holding one
        if (!magnetTarget || !ENTITY::DOES_ENTITY_EXIST(magnetTarget) || !holding)
        {
            Entity entity = 0;
            if (PLAYER::GET_ENTITY_PLAYER_IS_FREE_AIMING_AT(player, &entity)
                && ENTITY::DOES_ENTITY_EXIST(entity)
                && entity != playerPed
                && (!PED::IS_PED_A_PLAYER(entity) || entity == playerPed))
            {
                // If aiming at ped in vehicle, grab the vehicle (like force gun)
                if (ENTITY::IS_ENTITY_A_PED(entity)) {
                    Ped ped = (Ped)entity;
                    if (PED::IS_PED_IN_ANY_VEHICLE(ped, false)) {
                        Vehicle veh = PED::GET_VEHICLE_PED_IS_IN(ped, false);
                        if (ENTITY::DOES_ENTITY_EXIST(veh)) {
                            entity = veh;
                        }
                    }
                }
                magnetTarget = entity;
                holding = true;
                ENTITY::SET_ENTITY_HAS_GRAVITY(magnetTarget, false);
            }
            else {
                magnetTarget = 0;
                holding = false;
                launchTick = 0;
            }
        }
    }
    else
    {
        // Let go if not aiming
        if (magnetTarget && ENTITY::DOES_ENTITY_EXIST(magnetTarget))
            ENTITY::SET_ENTITY_HAS_GRAVITY(magnetTarget, true);
        magnetTarget = 0;
        holding = false;
        launchTick = 0;
    }

    // --- 2. If holding, move the entity in front of camera ---
    if (magnetTarget && holding && ENTITY::DOES_ENTITY_EXIST(magnetTarget))
    {
        Vector3 camPos = CAM::GET_GAMEPLAY_CAM_COORD();
        Vector3 camDir;
        {
            Vector3 rot = CAM::GET_GAMEPLAY_CAM_ROT(2);
            float radPitch = rot.x * 0.0174532924f;
            float radYaw = rot.z * 0.0174532924f;
            camDir.x = -sinf(radYaw) * cosf(radPitch);
            camDir.y = cosf(radYaw) * cosf(radPitch);
            camDir.z = sinf(radPitch);
        }
        float holdDist = magnetHoldDistance;
        Vector3 holdPos;
        holdPos.x = camPos.x + camDir.x * holdDist;
        holdPos.y = camPos.y + camDir.y * holdDist;
        holdPos.z = camPos.z + camDir.z * holdDist;

        Vector3 objPos = ENTITY::GET_ENTITY_COORDS(magnetTarget, true);
        Vector3 newVel;
        newVel.x = (holdPos.x - objPos.x) * magnetGripStrength;
        newVel.y = (holdPos.y - objPos.y) * magnetGripStrength;
        newVel.z = (holdPos.z - objPos.z) * magnetGripStrength;
        ENTITY::SET_ENTITY_VELOCITY(magnetTarget, newVel.x, newVel.y, newVel.z);

        // --- 3. Yeet Power: Launch if shooting (even while still holding/aiming) ---
        if (PED::IS_PED_SHOOTING(playerPed))
        {
            ENTITY::SET_ENTITY_HAS_GRAVITY(magnetTarget, true);

            // Calculate launch direction (from camera)
            launchDir = camDir;
            launchTick = GetTickCount();

            // Massive yeet
            ENTITY::SET_ENTITY_VELOCITY(
                magnetTarget,
                camDir.x * magnetLaunchPower,
                camDir.y * magnetLaunchPower,
                camDir.z * magnetLaunchPower
            );
            holding = false;         // <--- Let go, but leave magnetTarget for boost logic!
            // Do NOT set magnetTarget = 0 here!
        }
    }

    // --- 4. Always yeet on shoot, even if not holding ---
    if (!holding && PED::IS_PED_SHOOTING(playerPed))
    {
        Entity entity = 0;
        if (PLAYER::GET_ENTITY_PLAYER_IS_FREE_AIMING_AT(player, &entity)
            && ENTITY::DOES_ENTITY_EXIST(entity)
            && entity != playerPed
            && (!PED::IS_PED_A_PLAYER(entity) || entity == playerPed))
        {
            if (ENTITY::IS_ENTITY_A_PED(entity)) {
                Ped ped = (Ped)entity;
                if (PED::IS_PED_IN_ANY_VEHICLE(ped, false)) {
                    Vehicle veh = PED::GET_VEHICLE_PED_IS_IN(ped, false);
                    if (ENTITY::DOES_ENTITY_EXIST(veh)) {
                        entity = veh;
                    }
                }
            }
            Vector3 camPos = CAM::GET_GAMEPLAY_CAM_COORD();
            Vector3 camDir;
            Vector3 rot = CAM::GET_GAMEPLAY_CAM_ROT(2);
            float radPitch = rot.x * 0.0174532924f;
            float radYaw = rot.z * 0.0174532924f;
            camDir.x = -sinf(radYaw) * cosf(radPitch);
            camDir.y = cosf(radYaw) * cosf(radPitch);
            camDir.z = sinf(radPitch);

            ENTITY::SET_ENTITY_HAS_GRAVITY(entity, true);
            ENTITY::SET_ENTITY_VELOCITY(
                entity,
                camDir.x * magnetLaunchPower,
                camDir.y * magnetLaunchPower,
                camDir.z * magnetLaunchPower
            );

            // Start boost for extra force
            magnetTarget = entity;
            holding = false;
            launchDir = camDir;
            launchTick = GetTickCount();
            // Do NOT set magnetTarget = 0 here!
        }
    }

    // --- 5. Keep boosting the launched object for a few frames ---
    if (magnetTarget && !holding && ENTITY::DOES_ENTITY_EXIST(magnetTarget) && launchTick != 0)
    {
        DWORD now = GetTickCount();
        if (now - launchTick < 80)
        {
            ENTITY::APPLY_FORCE_TO_ENTITY(
                magnetTarget, 1,
                launchDir.x * magnetBoostPower,
                launchDir.y * magnetBoostPower,
                launchDir.z * magnetBoostPower,
                0, 0, 0, 0, false, true, true, false, true
            );
        }
        else
        {
            magnetTarget = 0;     // <--- Finally clear after boost!
            launchTick = 0;
        }
    }
}
void HandleSoulSwapGun()
{
    if (!soulSwapGun) return;
    Player player = PLAYER::PLAYER_ID();
    Ped playerPed = PLAYER::PLAYER_PED_ID();

    static DWORD lastSwapTick = 0;
    static Entity lastTarget = 0;

    // Check if player shot (last entity hit)
    if (PED::IS_PED_SHOOTING(playerPed)) {
        // Get aimed entity
        Entity aimed = 0;
        if (PLAYER::GET_ENTITY_PLAYER_IS_FREE_AIMING_AT(player, &aimed)
            && ENTITY::DOES_ENTITY_EXIST(aimed)
            && ENTITY::IS_ENTITY_A_PED(aimed)
            && aimed != playerPed
            && (lastTarget != aimed || GetTickCount() - lastSwapTick > 800)
            )
        {
            Hash pedModel = ENTITY::GET_ENTITY_MODEL(aimed);
            if (!STREAMING::IS_MODEL_VALID(pedModel)) return;

            // Filter (optional): skip invalid or unwanted models
            // if (PED::IS_PED_A_PLAYER((Ped)aimed)) return; // skip other players
            // more filters here if desired...

            // Request and load the model
            STREAMING::REQUEST_MODEL(pedModel);
            int tries = 50;
            while (!STREAMING::HAS_MODEL_LOADED(pedModel) && tries-- > 0) WAIT(10);

            if (STREAMING::HAS_MODEL_LOADED(pedModel)) {
                PLAYER::SET_PLAYER_MODEL(player, pedModel);
                Ped newPed = PLAYER::PLAYER_PED_ID();
                ENTITY::SET_ENTITY_INVINCIBLE(newPed, false);
                STREAMING::SET_MODEL_AS_NO_LONGER_NEEDED(pedModel);

                // Visual feedback
                UI::_SET_NOTIFICATION_TEXT_ENTRY("STRING");
                UI::_ADD_TEXT_COMPONENT_STRING("~b~Soul Swapped!");
                UI::_DRAW_NOTIFICATION(0, 0);

                lastTarget = aimed;
                lastSwapTick = GetTickCount();
            }
        }
    }
}
void Weapons_Init() {
    infAmmo = false; explosiveAmmo = false; fireAmmo = false; explosiveMelee = false; rapidFire = false; giveAllWeapons = false;
    forceGun = false; magnetGun = false; soulSwapGun = false;
    forceMultiplier = 10.0f; damageMultiplier = 1.0f; magnetGripStrength = 2.0f; magnetHoldDistance = 8.0f; magnetLaunchPower = 500.0f; magnetBoostPower = 400.0f;
}

// ---- Weapons Tab Tick ----
void Weapons_Tick() {
    Player p = PLAYER::PLAYER_ID();
    Ped ped = PLAYER::PLAYER_PED_ID();
    handleDamageMultiplier(p);
    if (infAmmo) {
        Hash cur; if (WEAPON::GET_CURRENT_PED_WEAPON(ped, &cur, 1)) {
            int maxAmmo = 0;
            if (WEAPON::GET_MAX_AMMO(ped, cur, &maxAmmo)) {
                WEAPON::SET_PED_AMMO(ped, cur, maxAmmo);
                int clipAmmo = WEAPON::GET_MAX_AMMO_IN_CLIP(ped, cur, 1);
                if (clipAmmo > 0) WEAPON::SET_AMMO_IN_CLIP(ped, cur, clipAmmo);
            }
        }
    }
    if (explosiveAmmo) GAMEPLAY::SET_EXPLOSIVE_AMMO_THIS_FRAME(p);
    if (giveAllWeapons) handleGiveAllWeapons(ped);
    if (fireAmmo) GAMEPLAY::SET_FIRE_AMMO_THIS_FRAME(p);
    if (explosiveMelee) GAMEPLAY::SET_EXPLOSIVE_MELEE_THIS_FRAME(p);
    if (rapidFire) handleRapidFire(ped);
    if (giveAllWeapons) handleGiveAllWeapons(ped);
    if (forceGun) HandleForceGun();
    if (magnetGun) HandleMagnetGun();
    if (soulSwapGun) HandleSoulSwapGun();
}

int Weapons_GetNumOptions() {
    return 15;
}

// FIX: Rewrote DrawMenu to use shared UI functions and match style
void Weapons_DrawMenu(int& menuIndex, float x, float y, float w, float h) {
    const char* weapOpts[] = { "Infinite Ammo", "Explosive Ammo", "Fire Ammo", "Explosive Melee", "Rapid Fire", "Give All Weapons", "Force Gun", "Magnet Gun", "Soul Swap Gun" };
    bool* weapToggles[] = { &infAmmo, &explosiveAmmo, &fireAmmo, &explosiveMelee, &rapidFire, &giveAllWeapons, &forceGun, &magnetGun, &soulSwapGun };
    int numToggles = sizeof(weapOpts) / sizeof(weapOpts[0]);

    const char* sliderOpts[] = { "Force Multiplier", "Damage Multiplier", "Magnet Grip", "Magnet Launch", "Magnet Boost", "Magnet Hold Distance" };
    float* sliderVals[] = { &forceMultiplier, &damageMultiplier, &magnetGripStrength, &magnetLaunchPower, &magnetBoostPower, &magnetHoldDistance };
    int numSliders = sizeof(sliderOpts) / sizeof(sliderOpts[0]);
    int totalOptions = numToggles + numSliders;

    char valueBuffer[32];
    for (int i = 0; i < totalOptions; ++i) {
        bool isSelected = (i == menuIndex);
        float currentY = y + h * i;

        if (i < numToggles) {
            // Draw all weapon options as standard toggles
            sprintf_s(valueBuffer, "%s", *weapToggles[i] ? "[ON]" : "[OFF]");
            DrawPairedMenuOption(weapOpts[i], valueBuffer, x, currentY, w, h, isSelected);
        }
        else {
            // Draw the sliders
            int sliderIndex = i - numToggles;
            sprintf_s(valueBuffer, "< %.1f >", *sliderVals[sliderIndex]);
            DrawPairedMenuOption(sliderOpts[sliderIndex], valueBuffer, x, currentY, w, h, isSelected);
        }
    }

    // --- Navigation and Activation Logic (CORRECTED) ---
    ClampMenuIndex(menuIndex, totalOptions);
    if (IsKeyJustUp(VK_NUMPAD8) || IsKeyJustUp(VK_UP) || PadPressed(DPAD_UP)) menuIndex = (menuIndex - 1 + totalOptions) % totalOptions;
    if (IsKeyJustUp(VK_NUMPAD2) || IsKeyJustUp(VK_DOWN) || PadPressed(DPAD_DOWN)) menuIndex = (menuIndex + 1) % totalOptions;

    // Activation now simply toggles the boolean for ALL weapon options
    if (IsKeyJustUp(VK_NUMPAD5) || IsKeyJustUp(VK_RETURN) || PadPressed(BTN_A)) {
        if (menuIndex < numToggles) {
            *weapToggles[menuIndex] = !*weapToggles[menuIndex];
        }
    }

    // --- Slider Adjustment Logic ---
    int direction = 0;
    // Corrected: Added IsKeyJustUp(VK_LEFT) and IsKeyJustUp(VK_RIGHT) for keyboard arrows
    if (IsKeyJustUp(VK_NUMPAD4) || IsKeyJustUp(VK_LEFT) || PadPressed(DPAD_LEFT))  direction = -1;
    if (IsKeyJustUp(VK_NUMPAD6) || IsKeyJustUp(VK_RIGHT) || PadPressed(DPAD_RIGHT)) direction = 1;

    if (direction != 0 && menuIndex >= numToggles) {
        int sliderIndex = menuIndex - numToggles;
        switch (sliderIndex) {
        case 0: forceMultiplier = std::max(0.0f, std::min(forceMultiplier + direction * 1.0f, 100.0f)); break;
        case 1: damageMultiplier = std::max(0.0f, std::min(damageMultiplier + direction * 1.0f, 100.0f)); break;
        case 2: magnetGripStrength = std::max(0.1f, std::min(magnetGripStrength + direction * 0.1f, 10.0f)); break;
        case 3: magnetLaunchPower = std::max(10.0f, std::min(magnetLaunchPower + direction * 10.0f, 2000.0f)); break;
        case 4: magnetBoostPower = std::max(10.0f, std::min(magnetBoostPower + direction * 10.0f, 2000.0f)); break;
        case 5: magnetHoldDistance = std::max(2.0f, std::min(magnetHoldDistance + direction * 0.5f, 20.0f)); break;
        }
    }
}