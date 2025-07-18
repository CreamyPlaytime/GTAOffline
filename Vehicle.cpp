#define _CRT_SECURE_NO_WARNINGS
#include "Vehicle.h"
#include "script.h" // For global inputDelayFrames, MENU_X, MENU_Y, etc.
#include "input.h"  // For IsKeyJustUp, PadPressed, etc.
#include <algorithm> // For std::max, std::min

// === Persistent Vehicle Options ===
bool godMode = false;
bool driveDeadCars = false;
bool vehicleFly = false;
bool autoRepair = false;
bool autoRepairNearby = false;
bool remoteControlEnabled = false;
bool remoteControlAllEnabled = false;

float impactForce = 1.0f;
float customSpeed = 1.0f;
float customTorque = 1.0f;
float customGravity = 1.0f;
float customTraction = 1.0f;
float vehicleDensity = 1.0f;

// === State variables for features ===
static Vehicle flyingVehicle = 0;
static float flyCurrentSpeed = 0.0f;
static Cam rcCam = 0;   // Remote Control Camera
static Vehicle remoteControlledVehicle = 0;
static float remoteCurrentSpeed = 0.0f;
static bool remoteControlActive = false;

// === Helper Functions ===
static Vector3 operator*(const Vector3& v, float s) {
    Vector3 result;
    result.x = v.x * s;
    result.y = v.y * s;
    result.z = v.z * s;
    return result;
}

void HandleRemoteControlFly();
void HandleRemoteControlFlyAll();

// ===== RC CAMERA CONTROL =====
void StartRemoteCam(Vehicle veh) {
    if (rcCam && CAM::DOES_CAM_EXIST(rcCam)) {
        CAM::DESTROY_CAM(rcCam, false);
        rcCam = 0;
    }
    Vector3 pos = ENTITY::GET_OFFSET_FROM_ENTITY_IN_WORLD_COORDS(veh, 0.0f, -8.0f, 4.0f); // 8 units behind, 4 up
    rcCam = CAM::CREATE_CAM("DEFAULT_SCRIPTED_CAMERA", true);
    CAM::SET_CAM_COORD(rcCam, pos.x, pos.y, pos.z);
    CAM::POINT_CAM_AT_ENTITY(rcCam, veh, 0.0f, 0.0f, 1.5f, true);
    CAM::SET_CAM_FOV(rcCam, 70.0f); // Standard FOV, change if you like
    CAM::RENDER_SCRIPT_CAMS(true, false, 0, true, false);
}

void StopRemoteCam() {
    if (rcCam && CAM::DOES_CAM_EXIST(rcCam)) {
        CAM::RENDER_SCRIPT_CAMS(false, false, 0, true, false);
        CAM::DESTROY_CAM(rcCam, false);
        rcCam = 0;
    }
}
static void RotationToDirection(const Vector3& rot, Vector3& outDir)
{
    float radX = rot.x * (float)M_PI / 180.0f; // Pitch
    float radZ = rot.z * (float)M_PI / 180.0f; // Heading/Yaw
    float cosX = cosf(radX);
    outDir.x = -sinf(radZ) * cosX;
    outDir.y = cosf(radZ) * cosX;
    outDir.z = sinf(radX);
}

// === MENU DISPATCHER ===
VehicleMenu g_vehicleMenu;
void VehicleMenu_DrawMenu(int& menuIndex, int& menuCategory) { g_vehicleMenu.DrawMenu(menuIndex, menuCategory); }
void (*Vehicle_DrawMenu)(int&, int&) = VehicleMenu_DrawMenu;

// Removed duplicate enum VehicleOption definition from here, it should only be in Vehicle.h

static const char* VEHOPT_LABELS[VEHOPT_COUNT] = {
    "God Mode", "Drive Dead Cars", "Vehicle Fly", "Auto Repair", "Auto Repair (Nearby)",
    "Impact Force", "Traction", "Engine Power", "Engine Torque", "Gravity", "Traffic Density",
    "Remote Control Vehicle", "Remote Control All Vehicles",
    "Repair Vehicle", "Back"
};

static const float SPEED_MIN = 1.0f, SPEED_MAX = 100.0f;
static const float TORQUE_MIN = 1.0f, TORQUE_MAX = 100.0f;
static const float GRAV_MIN = 0.1f, GRAV_MAX = 2.5f;
static const float FORCE_MIN = 1.0f, FORCE_MAX = 100.0f;
static const float TRACT_MIN = 0.1f, TRACT_MAX = 30.0f;
static const float DENSITY_MIN = 0.0f, DENSITY_MAX = 5.0f;

// === Helper: Pick Nearest Vehicle ===
// Modified to find nearest vehicle, but without excluding player's current vehicle
// Renamed to find_target_vehicle for clarity
Vehicle FindTargetVehicle() {
    Vector3 playerPos = ENTITY::GET_ENTITY_COORDS(PLAYER::PLAYER_PED_ID(), true);
    const int MAX_VEHICLES = 128;
    Vehicle vehicles[MAX_VEHICLES];
    int count = worldGetAllVehicles(vehicles, MAX_VEHICLES);

    float minDist = 999999.0f;
    Vehicle nearest = 0;
    // Iterate through all vehicles in the world, including the player's own, if applicable
    for (int i = 0; i < count; i++) {
        if (!ENTITY::DOES_ENTITY_EXIST(vehicles[i]))
            continue;
        Vector3 vPos = ENTITY::GET_ENTITY_COORDS(vehicles[i], true);
        float dist = sqrtf(powf(playerPos.x - vPos.x, 2) + powf(playerPos.y - vPos.y, 2) + powf(playerPos.z - vPos.z, 2));
        // Find the overall nearest vehicle, without excluding player's own
        if (dist < minDist) {
            minDist = dist;
            nearest = vehicles[i];
        }
    }
    return nearest;
}


// --- UI DRAWING HELPERS ---
// These helpers are already defined in script.cpp, just need to be declared extern if used here.
// However, since they are static in script.cpp, they are not directly accessible.
// They are typically in a shared UI.h or similar. For now, we'll assume they are available
// or that the compiler links them from script.obj.
// For the purpose of this file, we'll use the DrawPairedMenuOption and DrawMenuOption directly.

// === MENU CLASS ===
VehicleMenu::VehicleMenu() {}

Vehicle VehicleMenu::GetPlayerVehicle() {
    return PED::GET_VEHICLE_PED_IS_IN(PLAYER::PLAYER_PED_ID(), false);
}

void VehicleMenu::Tick()
{
    ApplyMods();
    HandleEnterDeadCar(); // This will now handle the "drive dead cars" logic
    HandleImpactForceIfActive();

    if (vehicleFly && PED::IS_PED_IN_ANY_VEHICLE(PLAYER::PLAYER_PED_ID(), false)) {
        HandleFlyIfActive();
    }
    else if (flyingVehicle != 0) {
        if (ENTITY::DOES_ENTITY_EXIST(flyingVehicle)) {
            VEHICLE::SET_VEHICLE_GRAVITY(flyingVehicle, true);
        }
        flyingVehicle = 0;
        flyCurrentSpeed = 0.0f;
    }

    // --- Remote Control Camera / Vehicle Management ---
    if (remoteControlEnabled) {
        // If remoteControlledVehicle is invalid or not set, try to find a new one
        if (!remoteControlledVehicle || !ENTITY::DOES_ENTITY_EXIST(remoteControlledVehicle)) {
            remoteControlledVehicle = FindTargetVehicle(); // Find the nearest vehicle
            if (remoteControlledVehicle) { // If a new vehicle was found, start its camera
                StartRemoteCam(remoteControlledVehicle);
                remoteControlActive = true; // Activate only if a vehicle is successfully targeted
            }
            else {
                // If no vehicle could be found nearby, temporarily deactivate remote control
                // to prevent constant attempts and ensure player control is restored
                remoteControlActive = false;
                PLAYER::SET_PLAYER_CONTROL(PLAYER::PLAYER_ID(), true, 0);
                StopRemoteCam();
            }
        }

        // Only run remote control logic if a vehicle is actively controlled
        if (remoteControlActive && ENTITY::DOES_ENTITY_EXIST(remoteControlledVehicle)) {
            HandleRemoteControlFly();
        }
    }
    else { // remoteControlEnabled is false
        // Ensure remote control is fully turned off and player control is restored
        if (remoteControlActive) { // If it was active, deactivate it gracefully
            PLAYER::SET_PLAYER_CONTROL(PLAYER::PLAYER_ID(), true, 0);
            if (remoteControlledVehicle && ENTITY::DOES_ENTITY_EXIST(remoteControlledVehicle)) {
                VEHICLE::SET_VEHICLE_GRAVITY(remoteControlledVehicle, true);
                ENTITY::SET_ENTITY_VELOCITY(remoteControlledVehicle, 0, 0, 0);
            }
            StopRemoteCam();
            remoteControlledVehicle = 0;
            remoteControlActive = false;
            remoteCurrentSpeed = 0.0f;
        }
    }

    if (remoteControlAllEnabled) {
        HandleRemoteControlFlyAll();
    }
}

void VehicleMenu::DrawMenu(int& menuIndex, int& menuCategory)
{
    extern const float MENU_X, MENU_Y, MENU_W, MENU_H; // From script.h
    extern int inputDelayFrames; // From script.h
    extern const RGBA BG_COLOR, HEADER_COLOR, OPTION_COLOR, SELECTED_COLOR, TEXT_COLOR, SELECTED_TEXT_COLOR, HEADER_TEXT_COLOR; // From script.h
    extern const int FONT; // From script.h
    extern void DrawMenuHeader(const char* text, float x, float y, float w); // From script.h
    extern void DrawMenuOption(const char* text, float x, float y, float w, float h, bool selected); // From script.h
    extern void DrawPairedMenuOption(const char* label, const char* value, float x, float y, float w, float h, bool selected); // From script.h
    extern void ClampMenuIndex(int& idx, int max); // From script.h


    const float x = MENU_X, y = MENU_Y, w = MENU_W, h = MENU_H;
    int numOptions = VEHOPT_COUNT;

    // Draw Background & Header
    float totalHeight = h * (numOptions + 1);
    GRAPHICS::DRAW_RECT(x + w * 0.5f, y - h * 0.5f + totalHeight * 0.5f, w, totalHeight, BG_COLOR.r, BG_COLOR.g, BG_COLOR.b, BG_COLOR.a);
    GRAPHICS::DRAW_RECT(x + w * 0.5f, y + h * 0.5f, w, h, HEADER_COLOR.r, HEADER_COLOR.g, HEADER_COLOR.b, HEADER_COLOR.a);

    UI::SET_TEXT_FONT(FONT);
    UI::SET_TEXT_SCALE(0.0f, 0.43f);
    UI::SET_TEXT_COLOUR(HEADER_TEXT_COLOR.r, HEADER_TEXT_COLOR.g, HEADER_TEXT_COLOR.b, HEADER_TEXT_COLOR.a);
    UI::SET_TEXT_CENTRE(true);
    UI::_SET_TEXT_ENTRY("STRING");
    UI::_ADD_TEXT_COMPONENT_STRING((char*)"Vehicle Mods");
    UI::_DRAW_TEXT(x + w * 0.5f, y + 0.005f);
    UI::SET_TEXT_CENTRE(false);

    // Draw Options
    float optionY = y + h;
    char valueBuffer[64];

    for (int i = 0; i < numOptions; ++i)
    {
        float currentY = optionY + h * i;
        bool isSelected = (i == menuIndex);
        const char* label = VEHOPT_LABELS[i];

        switch (i) {
        case VEHOPT_GODMODE:
        case VEHOPT_DRIVEDEAD:
        case VEHOPT_FLY:
        case VEHOPT_AUTOREPAIR:
        case VEHOPT_AUTOREPAIR_NEARBY:
        case VEHOPT_REMOTECONTROL:
        case VEHOPT_REMOTECONTROL_ALL: {
            bool* pBool = nullptr;
            if (i == VEHOPT_GODMODE) pBool = &godMode;
            else if (i == VEHOPT_DRIVEDEAD) pBool = &driveDeadCars;
            else if (i == VEHOPT_FLY) pBool = &vehicleFly;
            else if (i == VEHOPT_AUTOREPAIR) pBool = &autoRepair;
            else if (i == VEHOPT_AUTOREPAIR_NEARBY) pBool = &autoRepairNearby;
            else if (i == VEHOPT_REMOTECONTROL) pBool = &remoteControlEnabled;
            else if (i == VEHOPT_REMOTECONTROL_ALL) pBool = &remoteControlAllEnabled;
            sprintf_s(valueBuffer, "%s", *pBool ? "[ON]" : "[OFF]");
            DrawPairedMenuOption(label, valueBuffer, x, currentY, w, h, isSelected);
            break;
        }
        case VEHOPT_IMPACT_FORCE:
        case VEHOPT_TRACTION:
        case VEHOPT_ENGINE_POWER:
        case VEHOPT_TORQUE:
        case VEHOPT_GRAVITY:
        case VEHOPT_DENSITY: {
            float* pFloat = nullptr;
            if (i == VEHOPT_IMPACT_FORCE) pFloat = &impactForce;
            else if (i == VEHOPT_TRACTION) pFloat = &customTraction;
            else if (i == VEHOPT_ENGINE_POWER) pFloat = &customSpeed;
            else if (i == VEHOPT_TORQUE) pFloat = &customTorque;
            else if (i == VEHOPT_GRAVITY) pFloat = &customGravity;
            else if (i == VEHOPT_DENSITY) pFloat = &vehicleDensity;
            sprintf_s(valueBuffer, "< %.2f >", *pFloat);
            DrawPairedMenuOption(label, valueBuffer, x, currentY, w, h, isSelected);
            break;
        }
        case VEHOPT_REPAIR:
        case VEHOPT_BACK:
            DrawMenuOption(label, x, currentY, w, h, isSelected);
            break;
        }
    }

    // --- Navigation & Activation Logic ---
    // Only process input if no global input delay
    if (inputDelayFrames == 0) {
        // Corrected: Use IsKeyJustUp for single press navigation (for both numpad and arrows)
        bool up_pressed = IsKeyJustUp(VK_NUMPAD8) || IsKeyJustUp(VK_UP) || PadPressed(DPAD_UP);
        bool down_pressed = IsKeyJustUp(VK_NUMPAD2) || IsKeyJustUp(VK_DOWN) || PadPressed(DPAD_DOWN);

        if (up_pressed) {
            menuIndex = (menuIndex - 1 + numOptions) % numOptions;
            inputDelayFrames = 10; // Apply delay after navigation
        }
        if (down_pressed) {
            menuIndex = (menuIndex + 1) % numOptions;
            inputDelayFrames = 10; // Apply delay after navigation
        }

        // Corrected: Use IsKeyJustUp for single press slider adjustments (for both numpad and arrows)
        int direction = 0;
        if (IsKeyJustUp(VK_NUMPAD4) || IsKeyJustUp(VK_LEFT) || PadPressed(DPAD_LEFT))  direction = -1;
        if (IsKeyJustUp(VK_NUMPAD6) || IsKeyJustUp(VK_RIGHT) || PadPressed(DPAD_RIGHT)) direction = 1;

        if (direction != 0) { // Only adjust if a direction is pressed
            switch (menuIndex) {
            case VEHOPT_IMPACT_FORCE:   impactForce = std::max(FORCE_MIN, std::min(impactForce + direction * 1.0f, FORCE_MAX)); inputDelayFrames = 5; break;
            case VEHOPT_ENGINE_POWER:   customSpeed = std::max(SPEED_MIN, std::min(customSpeed + direction * 1.0f, SPEED_MAX)); inputDelayFrames = 5; break;
            case VEHOPT_TORQUE:         customTorque = std::max(TORQUE_MIN, std::min(customTorque + direction * 1.0f, TORQUE_MAX)); inputDelayFrames = 5; break;
            case VEHOPT_GRAVITY:        customGravity = std::max(GRAV_MIN, std::min(customGravity + direction * 0.1f, GRAV_MAX)); inputDelayFrames = 5; break;
            case VEHOPT_TRACTION:       customTraction = std::max(TRACT_MIN, std::min(customTraction + direction * 0.1f, TRACT_MAX)); inputDelayFrames = 5; break;
            case VEHOPT_DENSITY:        vehicleDensity = std::max(DENSITY_MIN, std::min(vehicleDensity + direction * 0.1f, DENSITY_MAX)); inputDelayFrames = 5; break;
            }
        }

        static bool prevA = false;
        bool currA = PadPressed(BTN_A);
        // Corrected: Added IsKeyJustUp(VK_RETURN) for selection
        if (IsKeyJustUp(VK_NUMPAD5) || IsKeyJustUp(VK_RETURN) || (currA && !prevA)) {
            switch (menuIndex) {
            case VEHOPT_GODMODE:           godMode = !godMode; inputDelayFrames = 10; break;
            case VEHOPT_DRIVEDEAD:         driveDeadCars = !driveDeadCars; inputDelayFrames = 10; break;
            case VEHOPT_FLY:
                vehicleFly = !vehicleFly;
                if (!vehicleFly) {
                    Vehicle currentVeh = GetPlayerVehicle();
                    if (currentVeh != 0 && ENTITY::DOES_ENTITY_EXIST(currentVeh)) {
                        VEHICLE::SET_VEHICLE_GRAVITY(currentVeh, true);
                    }
                }
                inputDelayFrames = 10; break;
            case VEHOPT_AUTOREPAIR:        autoRepair = !autoRepair; inputDelayFrames = 10; break;
            case VEHOPT_AUTOREPAIR_NEARBY: autoRepairNearby = !autoRepairNearby; inputDelayFrames = 10; break;
            case VEHOPT_REMOTECONTROL:
                remoteControlEnabled = !remoteControlEnabled;
                if (remoteControlEnabled) {
                    PLAYER::SET_PLAYER_CONTROL(PLAYER::PLAYER_ID(), false, 0);
                    // The next two lines are handled in VehicleMenu::Tick now for continuous re-attachment
                    // TryControlNearestVehicle();
                    // if (remoteControlledVehicle) StartRemoteCam(remoteControlledVehicle);
                }
                // The else block for turning off remote control is now handled in VehicleMenu::Tick
                // to centralize cleanup logic.
                inputDelayFrames = 10; break;
            case VEHOPT_REMOTECONTROL_ALL:
                remoteControlAllEnabled = !remoteControlAllEnabled;
                if (!remoteControlAllEnabled) {
                    const int MAX_VEHICLES = 128;
                    Vehicle vehicles[MAX_VEHICLES];
                    int count = worldGetAllVehicles(vehicles, MAX_VEHICLES);
                    for (int i = 0; i < count; ++i) {
                        if (ENTITY::DOES_ENTITY_EXIST(vehicles[i])) {
                            VEHICLE::SET_VEHICLE_GRAVITY(vehicles[i], true);
                        }
                    }
                }
                inputDelayFrames = 10; break;
            case VEHOPT_REPAIR:            Repair(); inputDelayFrames = 10; break;
            case VEHOPT_BACK:              menuCategory = 0; menuIndex = 3; inputDelayFrames = 10; break;
            }
        }
        prevA = currA;
    }
}
// ==========================
//       MODS & ACTIONS
// ==========================

void VehicleMenu::ApplyMods()
{
    VEHICLE::SET_PARKED_VEHICLE_DENSITY_MULTIPLIER_THIS_FRAME(vehicleDensity);
    VEHICLE::SET_RANDOM_VEHICLE_DENSITY_MULTIPLIER_THIS_FRAME(vehicleDensity);
    VEHICLE::SET_VEHICLE_DENSITY_MULTIPLIER_THIS_FRAME(vehicleDensity);

    Vehicle veh = GetPlayerVehicle();
    if (!veh) return;

    ENTITY::SET_ENTITY_INVINCIBLE(veh, godMode);
    VEHICLE::SET_VEHICLE_TYRES_CAN_BURST(veh, !godMode);
    VEHICLE::SET_VEHICLE_WHEELS_CAN_BREAK(veh, !godMode);
    VEHICLE::SET_VEHICLE_CAN_BE_VISIBLY_DAMAGED(veh, !godMode);
    VEHICLE::SET_VEHICLE_STRONG(veh, godMode);
    VEHICLE::SET_VEHICLE_EXPLODES_ON_HIGH_EXPLOSION_DAMAGE(veh, !godMode);
    VEHICLE::SET_DISABLE_VEHICLE_PETROL_TANK_DAMAGE(veh, godMode);
    VEHICLE::SET_DISABLE_VEHICLE_PETROL_TANK_FIRES(veh, godMode);

    if (driveDeadCars && ENTITY::IS_ENTITY_DEAD(veh)) {
        VEHICLE::SET_VEHICLE_ENGINE_ON(veh, true, true, true);
        VEHICLE::SET_VEHICLE_UNDRIVEABLE(veh, false);
    }

    VEHICLE::_SET_VEHICLE_ENGINE_POWER_MULTIPLIER(veh, customSpeed);
    VEHICLE::_SET_VEHICLE_ENGINE_TORQUE_MULTIPLIER(veh, customTorque);

    if (!vehicleFly && customGravity != 1.0f) {
        ENTITY::APPLY_FORCE_TO_ENTITY(veh, 1, 0.0f, 0.0f, (1.0f - customGravity) * 9.8f, 0.0f, 0.0f, 0.0f, 0, false, true, true, true, true);
    }

    // FIXED: Reverted to use the correct native for modifying traction as you pointed out.
    VEHICLE::SET_VEHICLE_FRICTION_OVERRIDE(veh, customTraction);

    if (autoRepair) {
        Repair();
    }
    if (autoRepairNearby) {
        const int MAX_VEHICLES = 128;
        Vehicle vehicles[MAX_VEHICLES];
        int count = worldGetAllVehicles(vehicles, MAX_VEHICLES);
        Vector3 playerPos = ENTITY::GET_ENTITY_COORDS(PLAYER::PLAYER_PED_ID(), true);

        for (int i = 0; i < count; i++) {
            if (ENTITY::DOES_ENTITY_EXIST(vehicles[i]) && vehicles[i] != veh) {
                Vector3 nearVehPos = ENTITY::GET_ENTITY_COORDS(vehicles[i], true);
                if (GAMEPLAY::GET_DISTANCE_BETWEEN_COORDS(playerPos.x, playerPos.y, playerPos.z, nearVehPos.x, nearVehPos.y, nearVehPos.z, true) < 20.0f) {
                    if (ENTITY::GET_ENTITY_HEALTH(vehicles[i]) < ENTITY::GET_ENTITY_MAX_HEALTH(vehicles[i])) {
                        VEHICLE::SET_VEHICLE_FIXED(vehicles[i]);
                        VEHICLE::SET_VEHICLE_DEFORMATION_FIXED(vehicles[i]);
                    }
                }
            }
        }
    }
}

void VehicleMenu::Repair()
{
    Vehicle veh = GetPlayerVehicle();
    if (!veh) return;
    VEHICLE::SET_VEHICLE_FIXED(veh);
    VEHICLE::SET_VEHICLE_DEFORMATION_FIXED(veh);
    VEHICLE::SET_VEHICLE_UNDRIVEABLE(veh, false);
    VEHICLE::SET_VEHICLE_ENGINE_ON(veh, true, true, true);
}

void VehicleMenu::Flip()
{
    Vehicle veh = GetPlayerVehicle();
    if (!veh) return;
    Vector3 rot = ENTITY::GET_ENTITY_ROTATION(veh, 2);
    rot.x = 0.0f; rot.y = 0.0f;
    ENTITY::SET_ENTITY_ROTATION(veh, rot.x, rot.y, rot.z, 2, 1);
}

void VehicleMenu::HandleImpactForceIfActive()
{
    if (impactForce <= 1.0f) return;
    Vehicle playerVeh = GetPlayerVehicle();
    if (!playerVeh) return;

    if (ENTITY::GET_ENTITY_SPEED(playerVeh) < 5.0f) return;

    if (ENTITY::HAS_ENTITY_COLLIDED_WITH_ANYTHING(playerVeh)) {
        const int MAX_VEHICLES = 128;
        Vehicle vehicles[MAX_VEHICLES];
        int count = worldGetAllVehicles(vehicles, MAX_VEHICLES);
        Vector3 playerVel = ENTITY::GET_ENTITY_VELOCITY(playerVeh);

        for (int i = 0; i < count; ++i) {
            Vehicle otherVeh = vehicles[i];
            if (!ENTITY::DOES_ENTITY_EXIST(otherVeh) || otherVeh == playerVeh) continue;

            if (ENTITY::IS_ENTITY_TOUCHING_ENTITY(playerVeh, otherVeh)) {
                ENTITY::APPLY_FORCE_TO_ENTITY(otherVeh, 1, playerVel.x * impactForce, playerVel.y * impactForce, playerVel.z, 0.0f, 0.0f, 0.0f, 0, false, true, true, false, true);
            }
        }
    }
}
void VehicleMenu::HandleEnterDeadCar() {
    if (!driveDeadCars) return;
    Ped playerPed = PLAYER::PLAYER_PED_ID();
    if (PED::IS_PED_IN_ANY_VEHICLE(playerPed, false)) return;

    if (CONTROLS::IS_CONTROL_JUST_PRESSED(0, ControlEnter)) {
        Vector3 pos = ENTITY::GET_ENTITY_COORDS(playerPed, true);
        Vehicle veh = VEHICLE::GET_CLOSEST_VEHICLE(pos.x, pos.y, pos.z, 5.0f, 0, 70);

        if (ENTITY::DOES_ENTITY_EXIST(veh) && ENTITY::IS_ENTITY_DEAD(veh)) {
            PED::SET_PED_INTO_VEHICLE(playerPed, veh, -1);
            VEHICLE::SET_VEHICLE_ENGINE_ON(veh, true, true, false);
        }
    }
}
void HandleRemoteControlFly()
{
    if (!remoteControlActive || !ENTITY::DOES_ENTITY_EXIST(remoteControlledVehicle)) return;

    if (remoteControlActive && rcCam && CAM::DOES_CAM_EXIST(rcCam) && ENTITY::DOES_ENTITY_EXIST(remoteControlledVehicle)) {
        Vector3 camPos = ENTITY::GET_OFFSET_FROM_ENTITY_IN_WORLD_COORDS(remoteControlledVehicle, 0.0f, -8.0f, 4.0f);
        CAM::SET_CAM_COORD(rcCam, camPos.x, camPos.y, camPos.z);
        CAM::POINT_CAM_AT_ENTITY(rcCam, remoteControlledVehicle, 0.0f, 0.0f, 1.5f, true);
    }

    Vector3 vehRot = ENTITY::GET_ENTITY_ROTATION(remoteControlledVehicle, 2);
    Vector3 fwdDir;
    RotationToDirection(vehRot, fwdDir);

    float leftX = GetPadAxis(LEFT_X);
    float leftY = GetPadAxis(LEFT_Y);
    float pitch = -leftY, yaw = -leftX, roll = 0.0f;
    if (IsKeyDown(VK_NUMPAD8) || IsKeyDown(VK_UP) || PadHeld(DPAD_UP))   pitch = 1.0f;
    if (IsKeyDown(VK_NUMPAD5) || IsKeyDown(VK_DOWN) || PadHeld(DPAD_DOWN)) pitch = -1.0f;
    if (IsKeyDown(VK_NUMPAD4) || IsKeyDown(VK_LEFT) || PadHeld(DPAD_LEFT))  yaw = -1.0f;
    if (IsKeyDown(VK_NUMPAD6) || IsKeyDown(VK_RIGHT) || PadHeld(DPAD_RIGHT)) yaw = 1.0f;
    if (IsKeyDown(VK_NUMPAD9) || PadHeld(BTN_RB))      roll = 1.0f;
    if (IsKeyDown(VK_NUMPAD7) || PadHeld(BTN_LB))      roll = -1.0f;

    vehRot.x += pitch * 1.8f;
    vehRot.y += roll * 1.8f;
    vehRot.z += yaw * 2.0f;
    if (vehRot.x > 89.9f) vehRot.x = 89.9f;
    if (vehRot.x < -89.9f) vehRot.x = -89.9f;
    ENTITY::SET_ENTITY_ROTATION(remoteControlledVehicle, vehRot.x, vehRot.y, vehRot.z, 2, 1);

    float thrust = 0.0f;
    if (RT_Held() || IsKeyDown('W')) thrust = 1.0f;
    if (g_padCurr.Gamepad.bLeftTrigger > 32 || IsKeyDown('S')) thrust = -1.0f;

    const float flyMaxSpeed = 84.0f;
    const float flyAcceleration = 32.0f;
    const float flyDrag = 0.985f;

    if (thrust != 0.0f) {
        remoteCurrentSpeed += flyAcceleration * thrust * GAMEPLAY::GET_FRAME_TIME();
    }
    else {
        remoteCurrentSpeed *= flyDrag;
    }

    remoteCurrentSpeed = std::max(-flyMaxSpeed, std::min(remoteCurrentSpeed, flyMaxSpeed));
    Vector3 vel = fwdDir * remoteCurrentSpeed;
    if (fabs(remoteCurrentSpeed) < 1.0f) {
        vel.z += 0.1f;
    }

    VEHICLE::SET_VEHICLE_GRAVITY(remoteControlledVehicle, false);
    ENTITY::SET_ENTITY_VELOCITY(remoteControlledVehicle, vel.x, vel.y, vel.z);

    if (IsKeyJustUp(VK_BACK) || IsKeyJustUp(VK_ESCAPE)) {
        PLAYER::SET_PLAYER_CONTROL(PLAYER::PLAYER_ID(), true, 0);
        VEHICLE::SET_VEHICLE_GRAVITY(remoteControlledVehicle, true);
        ENTITY::SET_ENTITY_VELOCITY(remoteControlledVehicle, 0, 0, 0);
        StopRemoteCam();
        remoteControlledVehicle = 0;
        remoteControlActive = false;
        remoteCurrentSpeed = 0.0f;
    }
}
void HandleRemoteControlFlyAll()
{
    const int MAX_VEHICLES = 128;
    Vehicle vehicles[MAX_VEHICLES];
    int count = worldGetAllVehicles(vehicles, MAX_VEHICLES);
    for (int i = 0; i < count; ++i) {
        if (!ENTITY::DOES_ENTITY_EXIST(vehicles[i])) continue;
        if (vehicles[i] == PED::GET_VEHICLE_PED_IS_IN(PLAYER::PLAYER_PED_ID(), false)) continue;

        Vector3 vehRot = ENTITY::GET_ENTITY_ROTATION(vehicles[i], 2);
        Vector3 fwdDir;
        RotationToDirection(vehRot, fwdDir);

        float leftX = GetPadAxis(LEFT_X);
        float leftY = GetPadAxis(LEFT_Y);
        float pitch = -leftY, yaw = -leftX, roll = 0.0f;
        if (IsKeyDown(VK_NUMPAD8) || IsKeyDown(VK_UP) || PadHeld(DPAD_UP))   pitch = 1.0f;
        if (IsKeyDown(VK_NUMPAD5) || IsKeyDown(VK_DOWN) || PadHeld(DPAD_DOWN)) pitch = -1.0f;
        if (IsKeyDown(VK_NUMPAD4) || IsKeyDown(VK_LEFT) || PadHeld(DPAD_LEFT))  yaw = -1.0f;
        if (IsKeyDown(VK_NUMPAD6) || IsKeyDown(VK_RIGHT) || PadHeld(DPAD_RIGHT)) yaw = 1.0f;
        if (IsKeyDown(VK_NUMPAD9) || PadHeld(BTN_RB))      roll = 1.0f;
        if (IsKeyDown(VK_NUMPAD7) || PadHeld(BTN_LB))      roll = -1.0f;

        vehRot.x += pitch * 1.8f;
        vehRot.y += roll * 1.8f;
        vehRot.z += yaw * 2.0f;
        if (vehRot.x > 89.9f) vehRot.x = 89.9f;
        if (vehRot.x < -89.9f) vehRot.x = -89.9f;
        ENTITY::SET_ENTITY_ROTATION(vehicles[i], vehRot.x, vehRot.y, vehRot.z, 2, 1);

        static float allRemoteSpeed = 0.0f;
        float thrust = 0.0f;
        if (RT_Held() || IsKeyDown('W')) thrust = 1.0f;
        if (g_padCurr.Gamepad.bLeftTrigger > 32 || IsKeyDown('S')) thrust = -1.0f;

        const float flyMaxSpeed = 84.0f;
        const float flyAcceleration = 32.0f;
        const float flyDrag = 0.985f;

        if (thrust != 0.0f) {
            allRemoteSpeed += flyAcceleration * thrust * GAMEPLAY::GET_FRAME_TIME();
        }
        else {
            allRemoteSpeed *= flyDrag;
        }
        allRemoteSpeed = std::max(-flyMaxSpeed, std::min(allRemoteSpeed, flyMaxSpeed));
        Vector3 vel = fwdDir * allRemoteSpeed;
        if (fabs(allRemoteSpeed) < 1.0f) vel.z += 0.1f;

        VEHICLE::SET_VEHICLE_GRAVITY(vehicles[i], false);
        ENTITY::SET_ENTITY_VELOCITY(vehicles[i], vel.x, vel.y, vel.z);
    }
}

void VehicleMenu::HandleFlyIfActive()
{
    Vehicle currentVeh = GetPlayerVehicle();

    if (currentVeh == 0) {
        if (flyingVehicle != 0 && ENTITY::DOES_ENTITY_EXIST(flyingVehicle)) {
            VEHICLE::SET_VEHICLE_GRAVITY(flyingVehicle, true);
        }
        flyingVehicle = 0;
        return;
    }

    if (currentVeh != flyingVehicle) {
        if (flyingVehicle != 0 && ENTITY::DOES_ENTITY_EXIST(flyingVehicle)) {
            VEHICLE::SET_VEHICLE_GRAVITY(flyingVehicle, true);
        }
        flyingVehicle = currentVeh;
        flyCurrentSpeed = 0.0f;
    }

    VEHICLE::SET_VEHICLE_GRAVITY(flyingVehicle, false);

    Vector3 vehRot = ENTITY::GET_ENTITY_ROTATION(flyingVehicle, 2);
    Vector3 fwdDir;
    RotationToDirection(vehRot, fwdDir);

    float leftX = GetPadAxis(LEFT_X);
    float leftY = GetPadAxis(LEFT_Y);
    float pitch = -leftY, yaw = -leftX, roll = 0.0f;
    if (IsKeyDown(VK_NUMPAD8) || IsKeyDown(VK_UP) || PadHeld(DPAD_UP))   pitch = 1.0f;
    if (IsKeyDown(VK_NUMPAD5) || IsKeyDown(VK_DOWN) || PadHeld(DPAD_DOWN)) pitch = -1.0f;
    if (IsKeyDown(VK_NUMPAD4) || IsKeyDown(VK_LEFT) || PadHeld(DPAD_LEFT))  yaw = -1.0f;
    if (IsKeyDown(VK_NUMPAD6) || IsKeyDown(VK_RIGHT) || PadHeld(DPAD_RIGHT)) yaw = 1.0f;
    if (IsKeyDown(VK_NUMPAD9) || PadHeld(BTN_RB))      roll = 1.0f;
    if (IsKeyDown(VK_NUMPAD7) || PadHeld(BTN_LB))      roll = -1.0f;

    vehRot.x += pitch * 1.8f;
    vehRot.y += roll * 1.8f;
    vehRot.z += yaw * 2.0f;
    if (vehRot.x > 89.9f) vehRot.x = 89.9f;
    if (vehRot.x < -89.9f) vehRot.x = -89.9f;
    ENTITY::SET_ENTITY_ROTATION(flyingVehicle, vehRot.x, vehRot.y, vehRot.z, 2, 1);

    float thrust = 0.0f;
    if (RT_Held() || IsKeyDown('W')) thrust = 1.0f;
    if (g_padCurr.Gamepad.bLeftTrigger > 32 || IsKeyDown('S')) thrust = -1.0f;

    const float flyMaxSpeed = 84.0f;
    const float flyAcceleration = 32.0f;
    const float flyDrag = 0.985f;

    if (thrust != 0.0f) {
        flyCurrentSpeed += flyAcceleration * thrust * GAMEPLAY::GET_FRAME_TIME();
    }
    else {
        flyCurrentSpeed *= flyDrag;
    }

    flyCurrentSpeed = std::max(-flyMaxSpeed, std::min(flyCurrentSpeed, flyMaxSpeed));
    Vector3 vel = fwdDir * flyCurrentSpeed;

    if (fabs(flyCurrentSpeed) < 1.0f) {
        vel.z += 0.0f;
    }

    ENTITY::SET_ENTITY_VELOCITY(flyingVehicle, vel.x, vel.y, vel.z);
}