#pragma once

#include "script.h" // Include script.h for RGBA struct and other common definitions
#include <vector>
#include <string> // For std::string in helper functions

// --- Settings Variables (Declared extern to be accessible from other files) ---
extern bool g_autoLoadCharacter;
extern bool g_showMoneyDisplay;
extern bool g_showRankBar;

// Keybinds
extern int g_keyboardMenuKey;
extern int g_controllerMenuButton1;
extern int g_controllerMenuButton2;

// UI Color Themes - REMOVED: g_colorThemes and g_currentThemeIndex

// --- Function Declarations ---
void Settings_Init(); // New initialization function for settings (now empty or minimal)
void SaveSettings();
void LoadSettings();
// Removed ApplyColorTheme();
void draw_settings_menu(); // Declares the function to draw the settings menu

// Helper functions for keybind names
std::string GetKeyName(int keyCode);
std::string GetControllerButtonName(int buttonCode);
