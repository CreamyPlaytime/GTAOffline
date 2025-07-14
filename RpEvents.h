#pragma once

#include <windows.h>
#include "script.h" // Included for native types like Vehicle

// --- Function Declarations ---
void RpEvents_Init();        // Call once on script start (e.g., load XP file, init)
void RpEvents_Tick();        // Call every frame, handles event detection and RP awarding
void RpEvents_DrawBar();     // Call every frame, draws the XP bar

void RpEvents_Reward(int amount, const char* msg = nullptr);
void RpEvents_Save(const char* path = "GTAOfflineXP.ini");
void RpEvents_Load(const char* path = "GTAOfflineXP.ini");

// --- XP/Level getters for UI ---
int RpEvents_GetXP();
int RpEvents_GetLevel();
int RpEvents_GetXPToNext();
int RpEvents_RecentRPGain();
ULONGLONG RpEvents_RecentRPGainTime();

// --- Constants ---
const int MAX_PLAYER_LEVEL = 8000; // Max player level is rank 8000.
