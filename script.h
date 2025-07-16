/*
	THIS FILE IS A PART OF GTA V SCRIPT HOOK SDK
				http://dev-c.com
			(C) Alexander Blade 2015
*/

#pragma once

#include "..\..\inc\natives.h"
#include "..\..\inc\types.h"
#include "..\..\inc\enums.h"

#include "..\..\inc\main.h"
#include "Properties.h"

// --- Defensive macros ---
#define VALID_PED(ped)     ((ped) != 0 && ENTITY::DOES_ENTITY_EXIST(ped))
#define VALID_PLAYER(p)   ((p) != -1 && PLAYER::IS_PLAYER_PLAYING(p))

// --- Struct for RGBA Colors ---
struct RGBA {
	int r, g, b, a;
};
extern const float MENU_X;
extern const float MENU_Y;
extern const float MENU_W;
extern const float MENU_H;
// --- Shared UI Color Palette (Declarations) ---
// These are now const again, as they will not be changed dynamically
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

// --- Shared Helper Functions (Declarations) ---
inline void ClampMenuIndex(int& idx, int max) {
	if (idx < 0) idx = 0;
	if (idx >= max) idx = max - 1;
}
void DrawMenuHeader(const char* text, float x, float y, float w);
void DrawMenuOption(const char* text, float x, float y, float w, float h, bool selected);
void DrawPairedMenuOption(const char* label, const char* value, float x, float y, float w, float h, bool selected);

// Global menu state variables - declared extern here
extern int menuCategory;
extern int menuIndex;
extern bool menuOpen;
extern int inputDelayFrames;

// Forward declaration of Category enum so Settings.cpp can use CAT_MAIN
enum Category;

void ScriptMain();
