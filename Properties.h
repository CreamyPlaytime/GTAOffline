/*
    THIS FILE IS A PART OF GTA Offline by CreamyPlaytime
    Built upon the GTA V Script Hook SDK: http://dev-c.com
    (C) CreamyPlaytime 2025 (and Alexander Blade 2015 for SDK components)
*/

#pragma once

#include "..\..\inc\natives.h"
#include "..\..\inc\types.h"
#include "..\..\inc\enums.h"
#include <vector>
#include <string> // For property names

// Define Virtual-Key codes for Y and N, if not already defined by windows.h
#ifndef VK_Y
#define VK_Y 0x59 // ASCII code for 'Y'
#endif
#ifndef VK_N
#define VK_N 0x4E // ASCII code for 'N'
#endif

// Enum for property types
enum PropertyType {
    RESIDENCE,
    BUSINESS
};

// Structure to hold property data
struct Property {
    std::string name;
    PropertyType type;      // Type of property (Residence or Business)
    Vector3 exteriorCoords; // Location of the "for sale" or "enter" marker outside
    float exteriorHeading;  // Player heading when teleported outside
    Vector3 interiorCoords; // Location inside the property
    float interiorHeading;  // Player heading when teleported inside
    int price;
    bool isBought;
    Hash blipSprite;        // Blip sprite for the property on the map
    int blipColorSale;      // Color of blip when for sale
    int blipColorOwned;     // Color of blip when owned
    Blip blipHandle;        // Handle for the map blip

    int interiorID;         // ID for the interior (if applicable)

    // Business-specific properties
    int generatedMoney;     // Money accumulated by the business
    int moneyCap;           // Maximum money the business can generate
    DWORD lastMoneyTickTime; // Last time money was generated
    Vector3 collectCoords;  // Location to collect money from business
    DWORD lastCapNotifyTime; // Last time max money notification was shown for this business

    // Passive RP specific properties
    DWORD lastRPTickTime;   // Last time RP was granted for residences

    // Daily Visit Bonus specific properties
    DWORD lastDailyBonusTime; // Last time daily bonus was collected for this business
};

// Global vector to store all properties
extern std::vector<Property> g_properties;

// Global variable to store the name of the last used house for spawning
extern std::string g_lastUsedHouseName;

// Global state for tracking if player is inside a property
extern bool g_isPlayerInsideProperty;
// CORRECTED: Add extern declarations for these global variables
extern int g_currentTeleportedInteriorID;
extern Property* g_activeTeleportedProperty;

// Function declarations for the property system
void Properties_Init();
void Properties_Tick();
void Properties_Save(const char* filename);
void Properties_Load(const char* filename);
void ShowNotification(const char* message); // Declared here for external use

// NEW: Function to reset all properties to default state and remove blips
void Properties_Reset();

// Declare external inputDelayFrames from script.cpp
extern int inputDelayFrames;