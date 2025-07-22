#pragma once

#include "script.h"
#include <vector>

// --- Struct for Owned Vehicle ---
struct OwnedVehicle {
    Hash hash;
    Blip blip;
    Vehicle vehicle_handle;
    DWORD invincible_until; // Added for spawn protection timer
};

// --- Function Declarations ---
void Garage_Init();
void Garage_Tick();
void draw_garage_menu();

// --- Garage Management Functions ---
void Garage_AddVehicle(Hash vehicleHash);
void Garage_Save();
void Garage_Load();
bool Garage_HasVehicle(Hash vehicleHash);

// ADDED: New function to check if a specific vehicle handle is player-owned.
bool Garage_IsVehicleOwned(Vehicle vehicle);