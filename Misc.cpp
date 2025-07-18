#define _CRT_SECURE_NO_WARNINGS
#define NOMINMAX
#include "Cheats.h"
#include "script.h"
#include "input.h"
#include <cstdio>
#include <cmath>
#include <windows.h>
#include <Xinput.h>
#include <algorithm>
#include <unordered_map>
#include <vector>
#include <ctime> // Needed for timestamp
#pragma comment(lib, "Xinput9_1_0.lib")

// FIXED: Define M_PI if it's not already defined to resolve compiler error.
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// ------ Bullet Explosion Types ------
const char* bulletTypeNames[] = {
    "Normal","Grenade","Grenade Launcher","Sticky Bomb","Molotov","Rocket","Tank Shell","Hi-Octane","Car","Plane",
    "Petrol Pump","Bike","Steam","Flame","Water Hydrant","Gas Canister","Boat","Ship Destroy","Truck","Bullet",
    "Smoke Launcher","Smoke Grenade","BZ Gas","Flare","Extinguisher","Programmable","Train","Barrel","Propane","Blimp",
    "Flame Explode","Tanker","Plane Rocket","Vehicle Bullet","Gas Tank","Bird Crap","Railgun","Blimp 2","Firework","Snowball",
    "Prox Mine","Valkyrie Cannon","Air Defence","Pipebomb","Vehicle Mine","Explosive Ammo","APC Shell","Cluster Bomb",
    "Bomb Gas","Incendiary","Bomb Standard","Torpedo","Torpedo Underwater","Bombushka Cannon","Cluster 2","Hunter Barrage",
    "Hunter Cannon","Rogue Cannon","Mine Underwater","Orbital Cannon","Bomb Std Wide","Explosive Shotgun","Oppressor2 Cannon",
    "Mortar Kinetic","VehMine Kinetic","VehMine EMP","VehMine Spike","VehMine Slick","VehMine Tar","Script Drone","Raygun",
    "Buried Mine","Script Missile","RCTank Rocket","Bomb Water","Bomb Water 2","Flashbang","Stun Grenade","Missile Large","Submarine Big","EMP Launcher"
};
static const int bulletTypeTags[] = {
    -1,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64,65,66,67,68,69,70,71,72,73,74,75,76,77,78,79,80
};

static int bulletTypeCount = sizeof(bulletTypeNames) / sizeof(bulletTypeNames[0]);
static DWORD lastBulletTick = 0, lastRapidFireTick = 0;

// ------ State Tracking for Cheats ------
// Removed 'relationshipsAltered' as we are no longer globally changing relationships
// static bool relationshipsAltered = false;
// --- NEW: Follower System State ---
static std::vector<Ped> g_followers;
static std::unordered_map<Ped, DWORD> followTaskTimes;


// ------ Misc Tab State ------
bool slowmo = false, refillHPArmor = false, moneyCheat = false;
int bulletExplosionType = 0;
bool hashGunActive = false;
bool wantedUp = false, wantedDown = false;
bool populateNow = false;
// NEW: Replaced allPedsFollowPlayer with a more detailed system
bool pedsFollowEnabled = false;
int followerCount = 5;
bool followAllPeds = false;
bool noHostilePeds = false;
bool noClipMode = false;
bool savePlayerCoords = false;
bool teleportPedsToPlayer = false; // This is now a continuous toggle
int  pedsToTeleportCount = 5;
static std::vector<Ped> teleportedPeds;
static DWORD lastPedTeleportTick = 0; // Cooldown for individual ped teleports
static const DWORD PED_TELEPORT_COOLDOWN = 1500; // 1.5 seconds between teleports


// --- Helper function for No Clip ---
static void GetCameraVectors(Vector3* fwd, Vector3* rgt, Vector3* up) {
    Vector3 rot = CAM::GET_GAMEPLAY_CAM_ROT(2);
    float rotZ = rot.z * (float)M_PI / 180.0f;
    float rotX = rot.x * (float)M_PI / 180.0f;
    float cosX = cos(rotX);

    // Corrected: Use -> operator for pointer access
    fwd->x = -sin(rotZ) * cosX;
    fwd->y = cos(rotZ) * cosX;
    fwd->z = sin(rotX);

    rgt->x = cos(rotZ);
    rgt->y = sin(rotZ);
    rgt->z = 0;

    up->x = fwd->y * rgt->z - fwd->z * rgt->y;
    up->y = fwd->z * rgt->x - fwd->x * rgt->z;
    up->z = fwd->x * rgt->y - fwd->y * rgt->x;
}

// Function to handle continuous ped teleportation
void handleTeleportPeds() {
    if (!teleportPedsToPlayer) return; // Only run if the toggle is ON

    Ped playerPed = PLAYER::PLAYER_PED_ID();
    Vector3 playerPos = ENTITY::GET_ENTITY_COORDS(playerPed, true);
    DWORD now = GetTickCount();

    // Clean up invalid peds from our list (e.g., dead or despawned)
    teleportedPeds.erase(std::remove_if(teleportedPeds.begin(), teleportedPeds.end(), [](Ped ped) {
        return !ENTITY::DOES_ENTITY_EXIST(ped) || PED::IS_PED_DEAD_OR_DYING(ped, true);
        }), teleportedPeds.end());

    // If we have fewer peds than desired and cooldown has passed, try to teleport more
    if (teleportedPeds.size() < pedsToTeleportCount && (now - lastPedTeleportTick) > PED_TELEPORT_COOLDOWN) {
        const int MAX_PEDS = 256; // Max peds to check in the world
        Ped allPeds[MAX_PEDS];
        int count = worldGetAllPeds(allPeds, MAX_PEDS);

        // Find one ped to teleport
        Ped pedToTeleport = 0;
        for (int i = 0; i < count; ++i) { // Iterate through all peds
            Ped currentPed = allPeds[i];

            // Skip player, dead peds, and peds already teleported by us
            if (currentPed == playerPed || !ENTITY::DOES_ENTITY_EXIST(currentPed) || PED::IS_PED_DEAD_OR_DYING(currentPed, true)) continue;

            // Ensure the ped is not a player or a mission ped
            if (PED::IS_PED_A_PLAYER(currentPed) || ENTITY::IS_ENTITY_A_MISSION_ENTITY(currentPed)) continue;

            bool alreadyManaged = false;
            for (const auto& tpPed : teleportedPeds) {
                if (tpPed == currentPed) {
                    alreadyManaged = true;
                    break;
                }
            }
            if (alreadyManaged) continue;

            pedToTeleport = currentPed; // Found a candidate
            break; // Only teleport one per cooldown period
        }

        if (pedToTeleport != 0) {
            // Clear existing tasks before teleporting to prevent conflicts
            AI::CLEAR_PED_TASKS_IMMEDIATELY(pedToTeleport);

            // Teleport the ped directly to the player's coordinates with a slightly larger Z offset
            Vector3 teleportTarget = playerPos;
            teleportTarget.z += 1.5f; // Increased Z offset to avoid nudging player

            ENTITY::SET_ENTITY_COORDS_NO_OFFSET(pedToTeleport, teleportTarget.x, teleportTarget.y, teleportTarget.z, false, false, true);

            // Attempt to make the ped persistent and give them a task
            PED::SET_PED_KEEP_TASK(pedToTeleport, true); // Try to keep their current task or allow new ones
            PED::SET_BLOCKING_OF_NON_TEMPORARY_EVENTS(pedToTeleport, true); // Block random events from interrupting
            PED::SET_PED_FLEE_ATTRIBUTES(pedToTeleport, 0, false); // Prevent them from fleeing
            AI::TASK_WANDER_IN_AREA(pedToTeleport, teleportTarget.x, teleportTarget.y, teleportTarget.z, 20.0f, 0.0f, 0); // Make them wander

            teleportedPeds.push_back(pedToTeleport); // Add to our list of managed peds
            lastPedTeleportTick = now; // Update cooldown
        }
        else {
            // Optional: Notify if a ped couldn't be found to teleport (e.g., no suitable peds nearby)
            // UI::_SET_NOTIFICATION_TEXT_ENTRY("STRING");
            // UI::_ADD_TEXT_COMPONENT_STRING("No suitable peds found for teleport.");
            // UI::_DRAW_NOTIFICATION(false, true);
        }
    }
}


// -- Populates World Now
static void POPULATE_NOW() { invoke<Void>(0x7472BB270D7B4F3E); }
static void handleBulletExplosion(Player player, Ped ped) {
    if (bulletExplosionType == 0) return;
    if (!WEAPON::IS_PED_ARMED(ped, 4)) return;
    bool shooting = KeyHeld(VK_LBUTTON) || RT_Held();
    static Vector3 lastImpact = { 0, 0, 0 };

    if (shooting) {
        Vector3 impactCoord;
        if (WEAPON::GET_PED_LAST_WEAPON_IMPACT_COORD(ped, &impactCoord)) {
            if (impactCoord.x != lastImpact.x || impactCoord.y != lastImpact.y || impactCoord.z != lastImpact.z) {
                int expType = bulletTypeTags[bulletExplosionType];
                if (expType >= 0) {
                    FIRE::ADD_EXPLOSION(impactCoord.x, impactCoord.y, impactCoord.z, expType, 2.0f, true, false, 1.1f);
                }
                lastImpact = impactCoord;
            }
        }
    }
}


// -- Hash Gun Logic --
void HandleHashGunNotification() {
    static Entity lastAimedEntity = 0;
    static Hash    lastLoggedHash = 0;
    static DWORD   lastNotifyTick = 0;
    static bool    hashVisible = false;
    Player player = PLAYER::PLAYER_ID();
    Ped    playerPed = PLAYER::PLAYER_PED_ID();
    if (PLAYER::IS_PLAYER_FREE_AIMING(player)) {
        Entity entity = 0;
        if (PLAYER::GET_ENTITY_PLAYER_IS_FREE_AIMING_AT(player, &entity) && ENTITY::DOES_ENTITY_EXIST(entity)) {
            Hash hash = ENTITY::GET_ENTITY_MODEL(entity);
            if ((entity != lastAimedEntity || hash != lastLoggedHash) && (GetTickCount() - lastNotifyTick > 500)) {
                char buf[128];
                sprintf(buf, "Hash: ~b~0x%08X~s~ | Handle: ~b~%d~s~ | Type: ~b~%d~s~", (unsigned int)hash, entity, ENTITY::GET_ENTITY_TYPE(entity));
                UI::_SET_NOTIFICATION_TEXT_ENTRY("STRING");
                UI::_ADD_TEXT_COMPONENT_STRING((char*)buf);
                UI::_DRAW_NOTIFICATION(false, false);
                lastAimedEntity = entity;
                lastLoggedHash = hash;
                lastNotifyTick = GetTickCount();
            }
            hashVisible = true;
        }
    }
    else {
        hashVisible = false;
    }
    if (lastAimedEntity && PED::IS_PED_SHOOTING(playerPed)) {
        char buf[256];
        sprintf(buf, "[HashGun] Shot entity handle: %d | hash: 0x%08X | type: %d", lastAimedEntity, (unsigned int)lastLoggedHash, ENTITY::GET_ENTITY_TYPE(lastAimedEntity));
        FILE* f = fopen("HashGun_Log.txt", "a");
        if (f) { fprintf(f, "%s\n", buf); fclose(f); }
        lastAimedEntity = 0;
    }
}

// ---- Misc Tab Init ----
void Misc_Init() {
    slowmo = refillHPArmor = moneyCheat = hashGunActive = wantedUp = wantedDown = populateNow = false;
    // NEW: Initialize follower system
    pedsFollowEnabled = false;
    followerCount = 5;
    followAllPeds = false;
    g_followers.clear();
    followTaskTimes.clear();

    noHostilePeds = false;
    noClipMode = false;
    savePlayerCoords = false;
    bulletExplosionType = 0;
    // Removed 'relationshipsAltered' init as it's no longer used for this feature
    // relationshipsAltered = false;
    teleportPedsToPlayer = false; // Initialize as false for the toggle
    pedsToTeleportCount = 5;
    teleportedPeds.clear(); // Ensure this is clear on init
}

// ---- Misc Tab Tick ----
void Misc_Tick() {
    Player p = PLAYER::PLAYER_ID();
    Ped ped = PLAYER::PLAYER_PED_ID();

    if (slowmo) GAMEPLAY::SET_TIME_SCALE(0.5f); else GAMEPLAY::SET_TIME_SCALE(1.0f);
    if (refillHPArmor) {
        ENTITY::SET_ENTITY_HEALTH(ped, ENTITY::GET_ENTITY_MAX_HEALTH(ped));
        PED::SET_PED_ARMOUR(ped, 100); refillHPArmor = false;
    }
    if (moneyCheat) {
        // Corrected: Use GAMEPLAY::GET_HASH_KEY for model comparison
        Hash model = GAMEPLAY::GET_HASH_KEY("player_zero");
        if (ENTITY::GET_ENTITY_MODEL(ped) == model ||
            ENTITY::GET_ENTITY_MODEL(ped) == GAMEPLAY::GET_HASH_KEY("player_one") ||
            ENTITY::GET_ENTITY_MODEL(ped) == GAMEPLAY::GET_HASH_KEY("player_two")) {
            for (int i = 0; i < 3; i++) {
                char statNameFull[32];
                sprintf(statNameFull, "SP%d_TOTAL_CASH", i);
                Hash hash = GAMEPLAY::GET_HASH_KEY(statNameFull);
                int val; STATS::STAT_GET_INT(hash, &val, -1); val += 1000000;
                STATS::STAT_SET_INT(hash, val, 1);
            }
        }
        moneyCheat = false;
    }
    if (wantedUp) {
        int wanted = PLAYER::GET_PLAYER_WANTED_LEVEL(p);
        if (wanted < 5) { PLAYER::SET_PLAYER_WANTED_LEVEL(p, wanted + 1, false); PLAYER::SET_PLAYER_WANTED_LEVEL_NOW(p, true); }
        wantedUp = false;
    }
    if (wantedDown) {
        int wanted = PLAYER::GET_PLAYER_WANTED_LEVEL(p);
        if (wanted > 0) { PLAYER::SET_PLAYER_WANTED_LEVEL(p, wanted - 1, false); PLAYER::SET_PLAYER_WANTED_LEVEL_NOW(p, true); }
        wantedDown = false;
    }
    if (hashGunActive) HandleHashGunNotification();
    if (populateNow) POPULATE_NOW();
    handleBulletExplosion(p, ped);

    if (savePlayerCoords) {
        Ped playerPed = PLAYER::PLAYER_PED_ID();
        if (ENTITY::DOES_ENTITY_EXIST(playerPed)) {
            Vector3 coords = ENTITY::GET_ENTITY_COORDS(playerPed, true);
            FILE* f = fopen("gtaofflinesavedcords.ini", "a");
            if (f) {
                time_t now = time(0);
                struct tm ltm;
                localtime_s(&ltm, &now);
                fprintf(f, "[%04d-%02d-%02d %02d:%02d:%02d] X=%.4f, Y=%.4f, Z=%.4f\n",
                    1900 + ltm.tm_year, 1 + ltm.tm_mon, ltm.tm_mday,
                    ltm.tm_hour, ltm.tm_min, ltm.tm_sec,
                    coords.x, coords.y, coords.z);
                fclose(f);
                UI::_SET_NOTIFICATION_TEXT_ENTRY("STRING");
                UI::_ADD_TEXT_COMPONENT_STRING("Coordinates saved to gtaofflinesavedcords.ini");
                UI::_DRAW_NOTIFICATION(false, true);
            }
        }
        savePlayerCoords = false;
    }

    // --- NEW: Follower System Logic ---
    if (pedsFollowEnabled) {
        Ped playerPed = PLAYER::PLAYER_PED_ID();
        DWORD now = GetTickCount();

        // 1. Prune invalid peds from our follower list
        g_followers.erase(std::remove_if(g_followers.begin(), g_followers.end(), [](Ped follower) {
            return !ENTITY::DOES_ENTITY_EXIST(follower) || PED::IS_PED_DEAD_OR_DYING(follower, 1);
            }), g_followers.end());

        // 2. Recruit new peds if needed
        int targetCount = followAllPeds ? 128 : followerCount;
        if (g_followers.size() < targetCount) {
            const int MAX_PEDS = 128;
            Ped peds[MAX_PEDS];
            int count = worldGetAllPeds(peds, MAX_PEDS);
            for (int i = 0; i < count && g_followers.size() < targetCount; ++i) {
                Ped currentPed = peds[i];
                if (ENTITY::DOES_ENTITY_EXIST(currentPed) && currentPed != playerPed && !PED::IS_PED_INJURED(currentPed)) {
                    // Check if this ped is already a follower
                    bool isAlreadyFollower = false;
                    for (const auto& follower : g_followers) {
                        if (follower == currentPed) {
                            isAlreadyFollower = true;
                            break;
                        }
                    }
                    if (!isAlreadyFollower) {
                        g_followers.push_back(currentPed); // Add to our list
                    }
                }
            }
        }

        // 3. Dismiss extra peds if count was lowered
        if (!followAllPeds && g_followers.size() > followerCount) {
            while (g_followers.size() > followerCount) {
                Ped pedToDismiss = g_followers.back();
                if (ENTITY::DOES_ENTITY_EXIST(pedToDismiss)) {
                    AI::CLEAR_PED_TASKS_IMMEDIATELY(pedToDismiss);
                    PED::SET_BLOCKING_OF_NON_TEMPORARY_EVENTS(pedToDismiss, false);
                }
                g_followers.pop_back();
            }
        }

        // 4. Task all current followers
        for (const auto& follower : g_followers) {
            PED::SET_BLOCKING_OF_NON_TEMPORARY_EVENTS(follower, true);
            PED::SET_PED_FLEE_ATTRIBUTES(follower, 0, false);
            bool shouldTask = false;
            if (followTaskTimes.find(follower) == followTaskTimes.end() || now - followTaskTimes[follower] > 2000) {
                AI::TASK_GO_TO_ENTITY(follower, playerPed, -1, 2.0f, 2.0f, 1073741824, 0);
                followTaskTimes[follower] = now;
            }
        }
    }
    else {
        // If feature is disabled, dismiss everyone
        if (!g_followers.empty()) {
            for (const auto& follower : g_followers) {
                if (ENTITY::DOES_ENTITY_EXIST(follower)) {
                    AI::CLEAR_PED_TASKS_IMMEDIATELY(follower);
                    PED::SET_BLOCKING_OF_NON_TEMPORARY_EVENTS(follower, false);
                }
            }
            g_followers.clear();
            followTaskTimes.clear();
        }
    }


    // --- No Hostile Peds (REVISED LOGIC: Always allow shooting, controls aggression/wanted) ---
    // This logic now ONLY affects wanted level and aggression, not targeting.
    if (noHostilePeds) { // When No Hostile Peds is ON: Peds are not hostile, no wanted level. Player can still shoot.
        // Prevent wanted level from increasing
        PLAYER::SET_WANTED_LEVEL_MULTIPLIER(0.0f);
        // Immediately clear any existing wanted level when the feature is enabled
        PLAYER::CLEAR_PLAYER_WANTED_LEVEL(p);
        PLAYER::SET_PLAYER_WANTED_LEVEL_NOW(p, true);

        // To make peds non-aggressive without breaking targeting,
        // we can iterate through nearby peds and set their combat attributes.
        // This is more performance intensive, but avoids the relationship group issue.
        // Only do this if 'noHostilePeds' is ON.
        const int MAX_PEDS_TO_CHECK = 100;
        Ped peds[MAX_PEDS_TO_CHECK];
        int count = worldGetAllPeds(peds, MAX_PEDS_TO_CHECK);
        Ped playerPed = PLAYER::PLAYER_PED_ID();

        for (int i = 0; i < count; ++i) {
            Ped currentPed = peds[i];
            if (ENTITY::DOES_ENTITY_EXIST(currentPed) && currentPed != playerPed && !PED::IS_PED_INJURED(currentPed)) {
                // Set peds to not attack the player
                PED::SET_PED_COMBAT_ATTRIBUTES(currentPed, 0, false); // CA_NormalCombat
                PED::SET_PED_COMBAT_ATTRIBUTES(currentPed, 1, false); // CA_Aggressive
                PED::SET_PED_COMBAT_ATTRIBUTES(currentPed, 2, false); // CA_AlwaysFight
                PED::SET_PED_COMBAT_ATTRIBUTES(currentPed, 3, false); // CA_BlockAttacks
                PED::SET_PED_FLEE_ATTRIBUTES(currentPed, 0, false); // Flee_Nothing
                PED::SET_PED_FLEE_ATTRIBUTES(currentPed, 2, true); // Flee_Smart (run away if attacked)
            }
        }
    }
    else { // When No Hostile Peds is OFF: Peds behave normally (some hostile, wanted applies). Player can still shoot.
        // Reset wanted level multiplier to allow it to increase normally
        PLAYER::SET_WANTED_LEVEL_MULTIPLIER(1.0f);

        // Reset combat attributes for nearby peds to allow normal aggression.
        const int MAX_PEDS_TO_CHECK = 100;
        Ped peds[MAX_PEDS_TO_CHECK];
        int count = worldGetAllPeds(peds, MAX_PEDS_TO_CHECK);
        Ped playerPed = PLAYER::PLAYER_PED_ID();

        for (int i = 0; i < count; ++i) {
            Ped currentPed = peds[i];
            if (ENTITY::DOES_ENTITY_EXIST(currentPed) && currentPed != playerPed && !PED::IS_PED_INJURED(currentPed)) {
                // Reset to default combat attributes (allow them to fight)
                PED::SET_PED_COMBAT_ATTRIBUTES(currentPed, 0, true); // CA_NormalCombat
                PED::SET_PED_COMBAT_ATTRIBUTES(currentPed, 1, true); // CA_Aggressive (if applicable to their model)
                PED::SET_PED_COMBAT_ATTRIBUTES(currentPed, 2, true); // CA_AlwaysFight (if applicable)
                PED::SET_PED_COMBAT_ATTRIBUTES(currentPed, 3, true); // CA_BlockAttacks (if applicable)
                PED::SET_PED_FLEE_ATTRIBUTES(currentPed, 0, true); // Flee_Nothing (allow them to stand ground)
                PED::SET_PED_FLEE_ATTRIBUTES(currentPed, 2, false); // Flee_Smart (disable specific flee behavior)
            }
        }
    }

    // --- Teleport Peds To You (Continuous Logic) ---
    // This function is now called every tick if the toggle is ON
    handleTeleportPeds();

    if (noClipMode) {
        if (ENTITY::DOES_ENTITY_EXIST(ped)) {
            ENTITY::SET_ENTITY_COLLISION(ped, false, false);
            Vector3 fwd, rgt, up;
            GetCameraVectors(&fwd, &rgt, &up);
            Vector3 vel = { 0, 0, 0 };
            float speed = 3.0f;
            if (KeyHeld(VK_LSHIFT) || PadHeld(BTN_A)) {
                speed = 10.0f;
            }
            if (KeyHeld('W')) { vel.x += fwd.x; vel.y += fwd.y; vel.z += fwd.z; }
            if (KeyHeld('S')) { vel.x -= fwd.x; vel.y -= fwd.y; vel.z -= fwd.z; }
            if (KeyHeld('A')) { vel.x -= rgt.x; vel.y -= rgt.y; }
            if (KeyHeld('D')) { vel.x += rgt.x; vel.y += rgt.y; }
            float leftY = GetPadAxis(LEFT_Y);
            float leftX = GetPadAxis(LEFT_X);
            if (abs(leftY) > 0.1f) {
                vel.x += fwd.x * leftY;
                vel.y += fwd.y * leftY;
                vel.z += fwd.z * leftY;
            }
            if (abs(leftX) > 0.1f) {
                vel.x += rgt.x * leftX;
                vel.y += rgt.y * leftX;
            }
            if (KeyHeld(VK_SPACE) || PadHeld(BTN_RB)) { vel.z += 1.0f; }
            if (KeyHeld(VK_LCONTROL) || PadHeld(BTN_LB)) { vel.z -= 1.0f; }
            ENTITY::SET_ENTITY_VELOCITY(ped, vel.x * speed, vel.y * speed, vel.z * speed);
        }
    }
}

int Misc_GetNumOptions() {
    return 16;
}

void Misc_DrawMenu(int& menuIndex, float x, float y, float w, float h) {
    // --- UPDATED: Added new options to the menu ---
    static const char* miscOpts[] = {
        "Slow Motion", "Refill Health/Armor", "Give $1,000,000", "Wanted Up", "Wanted Down",
        "Bullet Explosion Type", "Hash Gun", "Populate Now", "Peds Follow You", "Follower Count",
        "All Peds Follow You", "No Hostile Peds", "No Clip", "Save Current Coords",
        "Teleport Peds To You", // This is now a toggle
        "Teleport Count"
    };
    bool* miscToggles[] = {
        &slowmo, &refillHPArmor, &moneyCheat, &wantedUp, &wantedDown, nullptr, &hashGunActive, &populateNow,
        &pedsFollowEnabled, nullptr, &followAllPeds, &noHostilePeds, &noClipMode, &savePlayerCoords,
        &teleportPedsToPlayer, // Bind to the toggle variable
        nullptr
    };
    int miscNum = sizeof(miscOpts) / sizeof(miscOpts[0]);
    char valueBuffer[128];

    for (int i = 0; i < miscNum; ++i) {
        // Actions are options that reset after one use (or are explicitly triggered)
        // Note: Teleport Peds To You (index 14) is now a toggle, not an action in this context
        bool isAction = (i == 1 || i == 2 || i == 3 || i == 4 || i == 13);
        float currentY = y + h * i;
        bool isSelected = (i == menuIndex);

        if (i == 5) { // Bullet Explosion slider
            sprintf_s(valueBuffer, "< %s >", bulletTypeNames[bulletExplosionType]);
            DrawPairedMenuOption(miscOpts[i], valueBuffer, x, currentY, w, h, isSelected);
        }
        else if (i == 9) { // Follower Count slider
            sprintf_s(valueBuffer, "< %d >", followerCount);
            DrawPairedMenuOption(miscOpts[i], valueBuffer, x, currentY, w, h, isSelected);
        }
        else if (i == 15) { // Teleport Count slider
            sprintf_s(valueBuffer, "< %d >", pedsToTeleportCount);
            DrawPairedMenuOption(miscOpts[i], valueBuffer, x, currentY, w, h, isSelected);
        }
        else if (isAction) {
            DrawMenuOption(miscOpts[i], x, currentY, w, h, isSelected);
        }
        else { // Draw all others as toggles
            sprintf_s(valueBuffer, "%s", *miscToggles[i] ? "[ON]" : "[OFF]");
            DrawPairedMenuOption(miscOpts[i], valueBuffer, x, currentY, w, h, isSelected);
        }
    }

    // --- Navigation and Activation Logic ---
    ClampMenuIndex(menuIndex, miscNum);
    if (IsKeyJustUp(VK_NUMPAD8) || IsKeyJustUp(VK_UP) || PadPressed(DPAD_UP)) menuIndex = (menuIndex - 1 + miscNum) % miscNum;
    if (IsKeyJustUp(VK_NUMPAD2) || IsKeyJustUp(VK_DOWN) || PadPressed(DPAD_DOWN)) menuIndex = (menuIndex + 1) % miscNum;

    int dir = 0;
    if (IsKeyJustUp(VK_NUMPAD4) || IsKeyJustUp(VK_LEFT) || PadPressed(DPAD_LEFT))  dir = -1;
    if (IsKeyJustUp(VK_NUMPAD6) || IsKeyJustUp(VK_RIGHT) || PadPressed(DPAD_RIGHT)) dir = 1;

    if (dir != 0) {
        if (menuIndex == 5) {
            bulletExplosionType = (bulletExplosionType + dir + bulletTypeCount) % bulletTypeCount;
        }
        else if (menuIndex == 9 && !followAllPeds) {
            followerCount = std::max(1, std::min(followerCount + dir, 20));
        }
        else if (menuIndex == 15) { // Adjust teleport count
            pedsToTeleportCount = std::max(1, std::min(pedsToTeleportCount + dir, 50));
        }
    }

    if (IsKeyJustUp(VK_NUMPAD5) || IsKeyJustUp(VK_RETURN) || PadPressed(BTN_A)) {
        // Handle toggles
        if (menuIndex != 5 && menuIndex != 9 && menuIndex != 15 && miscToggles[menuIndex] != nullptr) {
            bool* pToggle = miscToggles[menuIndex];
            *pToggle = !*pToggle;

            // Specific logic for when teleportPedsToPlayer is toggled OFF
            if (pToggle == &teleportPedsToPlayer && !*pToggle) {
                teleportedPeds.clear(); // Clear the list when feature is turned off
            }

            if (pToggle == &noClipMode && !*pToggle) {
                Ped playerPed = PLAYER::PLAYER_PED_ID();
                if (ENTITY::DOES_ENTITY_EXIST(playerPed)) {
                    ENTITY::SET_ENTITY_COLLISION(playerPed, true, true);
                    Vector3 pos = ENTITY::GET_ENTITY_COORDS(playerPed, true);
                    float ground_z;
                    if (GAMEPLAY::GET_GROUND_Z_FOR_3D_COORD(pos.x, pos.y, pos.z, &ground_z, false)) {
                        pos.z = ground_z;
                        ENTITY::SET_ENTITY_COORDS_NO_OFFSET(playerPed, pos.x, pos.y, pos.z, 0, 0, 1);
                    }
                }
            }
        }
    }
}