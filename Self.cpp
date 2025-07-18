#include "Self.h"
#include "script.h"
#include "input.h"
#include <cmath>
#include <cstdio>
#include <algorithm> // Required for std::max

// --- Self Tab Toggles ---
bool playerGodMode = false, neverWanted = false, infStamina = false, seatbelt = false, teleportToWaypoint = false, superman = false;
bool ultraJump = false, infiniteJump = false, fastRun = false, fastSwim = false, noRagdoll = false, superJump = false;

// --- Superman Sub-power Toggles ---
bool supermanFlight = true;
bool laserEyes = true;
bool supermanImpact = true;

int  tpLastWaypoint = 0;

// ---- Teleport to Waypoint (toggle, auto on new marker) ----
void HandleTeleportToWaypoint() {
    if (!teleportToWaypoint) return;

    int blip = UI::GET_FIRST_BLIP_INFO_ID(8); // 8 = waypoint

    // Check if a new waypoint has been set since the last teleport
    if (UI::DOES_BLIP_EXIST(blip) && blip != tpLastWaypoint) {
        Vector3 coords = UI::GET_BLIP_COORDS(blip);
        float groundZ;

        // --- NEW LOGIC: Always teleport, but try to find ground first ---
        // 1. Attempt to find a safe ground Z coordinate.
        if (!GAMEPLAY::GET_GROUND_Z_FOR_3D_COORD(coords.x, coords.y, 1000.0f, &groundZ, false)) {
            // 2. If no ground is found, use the waypoint's own Z-coordinate as a fallback.
            //    This fulfills the request to always teleport.
            groundZ = coords.z;
        }

        // 3. Set the final Z coordinate slightly above the found ground/fallback position.
        coords.z = groundZ + 1.0f;

        Ped playerPed = PLAYER::PLAYER_PED_ID();
        Entity entityToTeleport = playerPed;

        // 4. Check if the player is in a vehicle and target it if so.
        if (PED::IS_PED_IN_ANY_VEHICLE(playerPed, false)) {
            entityToTeleport = PED::GET_VEHICLE_PED_IS_IN(playerPed, false);
        }

        // 5. Teleport the entity.
        ENTITY::SET_ENTITY_COORDS_NO_OFFSET(entityToTeleport, coords.x, coords.y, coords.z, 0, 0, 1);

        UI::_SET_NOTIFICATION_TEXT_ENTRY("STRING");
        UI::_ADD_TEXT_COMPONENT_STRING("~y~Teleported to Waypoint!");
        UI::_DRAW_NOTIFICATION(false, false);

        // Mark this waypoint as "used" to prevent continuous teleporting.
        tpLastWaypoint = blip;
    }
    else if (!UI::DOES_BLIP_EXIST(blip)) {
        // Reset when the player removes their waypoint.
        tpLastWaypoint = 0;
    }
}

// ---- Seatbelt Logic ----
void HandleSeatbelt() {
    Ped ped = PLAYER::PLAYER_PED_ID();
    if (!PED::IS_PED_IN_ANY_VEHICLE(ped, false)) return;

    Vehicle veh = PED::GET_VEHICLE_PED_IS_IN(ped, false);
    Hash model = ENTITY::GET_ENTITY_MODEL(veh);
    bool isBike = VEHICLE::IS_THIS_MODEL_A_BIKE(model) || VEHICLE::IS_THIS_MODEL_A_BICYCLE(model) || VEHICLE::IS_THIS_MODEL_A_QUADBIKE(model);

    PED::SET_PED_CAN_BE_DRAGGED_OUT(ped, !seatbelt);
    PED::SET_PED_CONFIG_FLAG(ped, 32, !seatbelt);    // Inverted!
    PED::SET_PED_CAN_RAGDOLL_FROM_PLAYER_IMPACT(ped, !seatbelt);
    PED::SET_PED_CONFIG_FLAG(ped, 29, seatbelt);

    if (isBike)
        PED::SET_PED_CAN_BE_KNOCKED_OFF_VEHICLE(ped, seatbelt ? 0 : 1);
}


// ---- Infinite Stamina ----
void HandleInfiniteStamina() {
    if (!infStamina) return;
    Player p = PLAYER::PLAYER_ID();
    PLAYER::RESTORE_PLAYER_STAMINA(p, 1.0f);
}

// --- Tunable Parameters ---
static float supermanSpeed = 35.0f;
static float supermanBoost = 10.0f;
static float supermanImpactForce = 55.0f;
static float supermanMeleeForce = 80.0f;

// --- State Management ---
static bool isFlyingLastFrame = false;

// --- Helper function to get camera direction ---
Vector3 get_cam_direction() {
    Vector3 rot = CAM::GET_GAMEPLAY_CAM_ROT(2);
    float radPitch = rot.x * 0.0174532924f;
    float radYaw = rot.z * 0.0174532924f;
    float cosPitch = cosf(radPitch);

    Vector3 dir;
    dir.x = -sinf(radYaw) * cosPitch;
    dir.y = cosf(radYaw) * cosPitch;
    dir.z = sinf(radPitch);
    return dir;
}

// --- NEW: Laser Eyes Logic ---
void handleLaserEyes(Ped ped, Player p) {
    const int ControlAim = 25;
    const int ControlAttack = 24;

    // Allow aiming without a weapon
    PLAYER::SET_PLAYER_SIMULATE_AIMING(p, true);

    // If player is aiming (LT or Right Mouse)
    if (CONTROLS::IS_DISABLED_CONTROL_PRESSED(0, ControlAim)) {
        UI::SHOW_HUD_COMPONENT_THIS_FRAME(14); // HudComponentReticle

        // Get eye positions with offsets
        Vector3 rightEyePos = PED::GET_PED_BONE_COORDS(ped, 31086, 0.07f, 0.03f, 0.03f);
        Vector3 leftEyePos = PED::GET_PED_BONE_COORDS(ped, 31086, -0.07f, 0.03f, 0.03f);

        // Raycast from camera to find the target point
        Vector3 camPos = CAM::GET_GAMEPLAY_CAM_COORD();
        Vector3 camDir = get_cam_direction();
        Vector3 rayEnd;
        rayEnd.x = camPos.x + camDir.x * 1000.0f;
        rayEnd.y = camPos.y + camDir.y * 1000.0f;
        rayEnd.z = camPos.z + camDir.z * 1000.0f;

        int rayHandle = WORLDPROBE::_CAST_RAY_POINT_TO_POINT(camPos.x, camPos.y, camPos.z, rayEnd.x, rayEnd.y, rayEnd.z, -1, ped, 7);
        BOOL didHit;
        Vector3 hitCoords, surfaceNormal;
        Entity hitEntity;
        WORLDPROBE::_GET_RAYCAST_RESULT(rayHandle, &didHit, &hitCoords, &surfaceNormal, &hitEntity);

        Vector3 targetPos = didHit ? hitCoords : rayEnd;

        // Draw two laser beams
        GRAPHICS::DRAW_LINE(leftEyePos.x, leftEyePos.y, leftEyePos.z, targetPos.x, targetPos.y, targetPos.z, 255, 24, 24, 255);
        GRAPHICS::DRAW_LINE(rightEyePos.x, rightEyePos.y, rightEyePos.z, targetPos.x, targetPos.y, targetPos.z, 255, 24, 24, 255);

        // If player presses fire, create an explosion
        if (CONTROLS::IS_DISABLED_CONTROL_JUST_PRESSED(0, ControlAttack)) {
            if (didHit) {
                FIRE::ADD_EXPLOSION(targetPos.x, targetPos.y, targetPos.z, 29, 2.5f, true, false, 1.0f); // Railgun explosion
            }
        }
    }
    else {
        PLAYER::SET_PLAYER_SIMULATE_AIMING(p, false);
    }
}


// --- Superman Flight Logic ---
static void handleSupermanFlight(Ped ped, Player p) {
    if (PED::IS_PED_IN_ANY_VEHICLE(ped, false) || PED::IS_PED_RAGDOLL(ped)) return;

    bool isFlyingKeyPressed = KeyHeld(VK_SPACE) || PadHeld(BTN_X);
    bool isBoosting = KeyHeld(VK_SHIFT) || PadHeld(BTN_RB);

    if (isFlyingKeyPressed) {
        isFlyingLastFrame = true;
        PED::SET_PED_CAN_RAGDOLL(ped, false);
        PED::SET_PED_GRAVITY(ped, false);

        Vector3 camDir = get_cam_direction();

        Vector3 vel;
        vel.x = camDir.x * supermanSpeed;
        vel.y = camDir.y * supermanSpeed;
        vel.z = camDir.z * supermanSpeed;

        if (isBoosting) {
            vel.z += supermanBoost;
        }
        ENTITY::SET_ENTITY_VELOCITY(ped, vel.x, vel.y, vel.z);
    }
    else if (isFlyingLastFrame) {
        PED::SET_PED_GRAVITY(ped, true);
        PED::SET_PED_CAN_RAGDOLL(ped, true);
        isFlyingLastFrame = false;
    }
}

// --- Impact Force (Walking or Flying) ---
static void handleSupermanImpactForce(Ped ped) {
    if (PED::IS_PED_IN_ANY_VEHICLE(ped, false)) return;

    if (ENTITY::HAS_ENTITY_COLLIDED_WITH_ANYTHING(ped)) {
        Vector3 playerVel = ENTITY::GET_ENTITY_VELOCITY(ped);
        float speed = sqrtf(playerVel.x * playerVel.x + playerVel.y * playerVel.y + playerVel.z * playerVel.z);

        if (speed > 0.1f) {
            Vector3 dir = { playerVel.x / speed, playerVel.y / speed, playerVel.z / speed };

            auto applyImpactToTouchedEntity = [&](Entity targetEnt) {
                if (ENTITY::DOES_ENTITY_EXIST(targetEnt) && targetEnt != ped) {
                    if (ENTITY::IS_ENTITY_TOUCHING_ENTITY(ped, targetEnt)) {
                        ENTITY::APPLY_FORCE_TO_ENTITY(targetEnt, 1, dir.x * supermanImpactForce, dir.y * supermanImpactForce, dir.z * supermanImpactForce, 0.0f, 0.0f, 0.0f, 0, false, true, true, false, true);
                    }
                }
                };

            const int MAX_ENTITIES = 128;
            Entity entities[MAX_ENTITIES];

            int found = worldGetAllVehicles(entities, MAX_ENTITIES);
            for (int i = 0; i < found; i++) applyImpactToTouchedEntity(entities[i]);

            found = worldGetAllPeds(entities, MAX_ENTITIES);
            for (int i = 0; i < found; i++) applyImpactToTouchedEntity(entities[i]);

            found = worldGetAllObjects(entities, MAX_ENTITIES);
            for (int i = 0; i < found; i++) applyImpactToTouchedEntity(entities[i]);
        }
    }
}

// --- Super Melee Impact ---
static void handleSupermanMelee(Ped ped) {
    if (!PED::IS_PED_ON_FOOT(ped)) return;

    const int MAX_PEDS = 128;
    Ped peds[MAX_PEDS];
    int count = worldGetAllPeds(peds, MAX_PEDS);

    for (int i = 0; i < count; i++) {
        Ped targetPed = peds[i];
        if (targetPed != ped && ENTITY::DOES_ENTITY_EXIST(targetPed)) {
            if (ENTITY::HAS_ENTITY_BEEN_DAMAGED_BY_ENTITY(targetPed, ped, true)) {
                ENTITY::CLEAR_ENTITY_LAST_DAMAGE_ENTITY(targetPed);
                Vector3 forwardVec = ENTITY::GET_ENTITY_FORWARD_VECTOR(ped);
                ENTITY::APPLY_FORCE_TO_ENTITY(
                    targetPed, 1,
                    forwardVec.x * supermanMeleeForce,
                    forwardVec.y * supermanMeleeForce,
                    20.0f,
                    0.0f, 0.0f, 0.0f, 0, false, true, true, false, true
                );
                break;
            }
        }
    }
}

// --- Ultra Jump/Infinite Jump ---
static DWORD lastUltraJumpTick = 0;
static void handleUltraJump(Ped ped, Player p) {
    if (!ultraJump) return;
    if (!PED::IS_PED_ON_FOOT(ped) || PED::IS_PED_RAGDOLL(ped) || PED::IS_PED_IN_ANY_VEHICLE(ped, false))
        return;
    bool isJumping = KeyHeld(VK_SPACE) || PadHeld(BTN_X);

    if (isJumping) {
        GAMEPLAY::SET_SUPER_JUMP_THIS_FRAME(p);

        DWORD now = GetTickCount();
        if (now - lastUltraJumpTick > 300) {
            Vector3 vel = ENTITY::GET_ENTITY_VELOCITY(ped);
            if (vel.z < 12.0f) {
                ENTITY::SET_ENTITY_VELOCITY(ped, vel.x, vel.y, vel.z + 7.8f);
            }
            AI::TASK_JUMP(ped, true);
            lastUltraJumpTick = now;
        }
    }
}

static DWORD lastInfiniteJumpTick = 0;
static void handleInfiniteJump(Ped ped, Player p) {
    if (!infiniteJump) return;
    if (!PED::IS_PED_ON_FOOT(ped) || PED::IS_PED_RAGDOLL(ped) || PED::IS_PED_IN_ANY_VEHICLE(ped, false))
        return;
    bool isJumping = KeyHeld(VK_SPACE) || PadHeld(BTN_X);
    if (isJumping) {
        GAMEPLAY::SET_SUPER_JUMP_THIS_FRAME(p);
        DWORD now = GetTickCount();
        if (now - lastInfiniteJumpTick > 350) {
            AI::TASK_JUMP(ped, true);
            lastInfiniteJumpTick = now;
        }
    }
}

void Self_Init() {
    playerGodMode = neverWanted = infStamina = seatbelt = teleportToWaypoint = superman =
        ultraJump = fastRun = fastSwim = noRagdoll = superJump = infiniteJump = false;

    supermanFlight = true;
    laserEyes = true;
    supermanImpact = true;

    tpLastWaypoint = 0;
}

void Self_Tick() {
    Player p = PLAYER::PLAYER_ID();
    Ped ped = PLAYER::PLAYER_PED_ID();

    PLAYER::SET_PLAYER_INVINCIBLE(p, playerGodMode);
    if (neverWanted) { PLAYER::CLEAR_PLAYER_WANTED_LEVEL(p); PLAYER::SET_MAX_WANTED_LEVEL(0); }
    else { PLAYER::SET_MAX_WANTED_LEVEL(5); }
    if (infStamina) HandleInfiniteStamina();
    if (seatbelt) HandleSeatbelt();
    if (teleportToWaypoint) HandleTeleportToWaypoint();
    if (fastRun) PLAYER::SET_RUN_SPRINT_MULTIPLIER_FOR_PLAYER(p, 1.49f); else PLAYER::SET_RUN_SPRINT_MULTIPLIER_FOR_PLAYER(p, 1.0f);
    if (fastSwim) PLAYER::SET_SWIM_MULTIPLIER_FOR_PLAYER(p, 1.49f); else PLAYER::SET_SWIM_MULTIPLIER_FOR_PLAYER(p, 1.0f);
    PED::SET_PED_CAN_RAGDOLL(ped, !noRagdoll);

    if (superman) {
        PLAYER::SET_PLAYER_MELEE_WEAPON_DAMAGE_MODIFIER(p, 100.0f);
        if (supermanFlight) handleSupermanFlight(ped, p);
        if (supermanImpact) {
            handleSupermanImpactForce(ped);
            handleSupermanMelee(ped);
        }
        if (laserEyes) handleLaserEyes(ped, p);
        else PLAYER::SET_PLAYER_SIMULATE_AIMING(p, false);

        if (!supermanFlight && isFlyingLastFrame) {
            PED::SET_PED_GRAVITY(ped, true);
            PED::SET_PED_CAN_RAGDOLL(ped, true);
            isFlyingLastFrame = false;
        }
    }
    else {
        if (isFlyingLastFrame) {
            PED::SET_PED_GRAVITY(ped, true);
            PED::SET_PED_CAN_RAGDOLL(ped, true);
            isFlyingLastFrame = false;
        }
        PLAYER::SET_PLAYER_MELEE_WEAPON_DAMAGE_MODIFIER(p, 1.0f);
        PLAYER::SET_PLAYER_SIMULATE_AIMING(p, false);
    }

    if (ultraJump) handleUltraJump(ped, p);
    if (superJump) GAMEPLAY::SET_SUPER_JUMP_THIS_FRAME(p);
    if (infiniteJump) handleInfiniteJump(ped, p);
}

int Self_GetNumOptions() {
    return superman ? 15 : 12;
}

// FIX: Rewrote DrawMenu to use shared UI functions and match style
void Self_DrawMenu(int& menuIndex, float x, float y, float w, float h) {
    static const char* selfOpts_Normal[] = { "God Mode", "Never Wanted", "Infinite Stamina", "Seatbelt", "TP to Waypoint", "Superman", "Super Jump", "Infinite Hop", "Ultra Jump", "Fast Run", "Fast Swim", "No Ragdoll" };
    bool* selfToggles_Normal[] = { &playerGodMode, &neverWanted, &infStamina, &seatbelt, &teleportToWaypoint, &superman, &superJump, &infiniteJump, &ultraJump, &fastRun, &fastSwim, &noRagdoll };
    int selfNum_Normal = sizeof(selfOpts_Normal) / sizeof(selfOpts_Normal[0]);
    static const char* selfOpts_Superman[] = { "God Mode", "Never Wanted", "Infinite Stamina", "Seatbelt", "TP to Waypoint", "Superman", "  > Superman Flight", "  > Laser Eyes", "  > Superman Impact", "Super Jump", "Infinite Hop", "Ultra Jump", "Fast Run", "Fast Swim", "No Ragdoll" };
    bool* selfToggles_Superman[] = { &playerGodMode, &neverWanted, &infStamina, &seatbelt, &teleportToWaypoint, &superman, &supermanFlight, &laserEyes, &supermanImpact, &superJump, &infiniteJump, &ultraJump, &fastRun, &fastSwim, &noRagdoll };
    int selfNum_Superman = sizeof(selfOpts_Superman) / sizeof(selfOpts_Superman[0]);

    const char** currentOpts = superman ? selfOpts_Superman : selfOpts_Normal;
    bool** currentToggles = superman ? selfToggles_Superman : selfToggles_Normal;
    int currentNum = superman ? selfNum_Superman : selfNum_Normal;

    for (int i = 0; i < currentNum; ++i) {
        char value[16];
        sprintf_s(value, "%s", *currentToggles[i] ? "[ON]" : "[OFF]");
        DrawPairedMenuOption(currentOpts[i], value, x, y + h * i, w, h, i == menuIndex);
    }

    ClampMenuIndex(menuIndex, currentNum);

    if (IsKeyJustUp(VK_NUMPAD8) || IsKeyJustUp(VK_UP) || PadPressed(DPAD_UP)) menuIndex = (menuIndex - 1 + currentNum) % currentNum; //
    if (IsKeyJustUp(VK_NUMPAD2) || IsKeyJustUp(VK_DOWN) || PadPressed(DPAD_DOWN)) menuIndex = (menuIndex + 1) % currentNum; //
    if (IsKeyJustUp(VK_NUMPAD5) || IsKeyJustUp(VK_RETURN) || PadPressed(BTN_A)) { //
        bool* togglePtr = currentToggles[menuIndex];
        *togglePtr = !(*togglePtr);
        if (togglePtr == &superman && !superman && menuIndex >= selfNum_Normal) {
            menuIndex = selfNum_Normal - 1;
        }
    }
}