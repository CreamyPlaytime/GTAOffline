#include "CharacterCreator.h"
#include "script.h"
#include "input.h"
#include <cstdio>
#include <cstring>
#include <cctype>
#include <string> // Required for std::to_string

// Clothing slot indices (components)
#define SLOT_UNDERSHIRT 8
#define SLOT_TOP        11
#define SLOT_LEGS       4
#define SLOT_SHOES      6
#define SLOT_SPECIAL    7 // Misc. Accessory slot

// Prop slot indices (props)
#define PROP_SLOT_HAT       0
#define PROP_SLOT_GLASSES   1

// Internal identifier for mask component within CharacterCreator_NextProp/Texture
// This value is chosen to be distinct from PROP_SLOT_HAT, PROP_SLOT_GLASSES, and SLOT_SPECIAL
// It does NOT represent the actual GTA component ID, which is handled inside CharacterCreator_Apply().
#define INTERNAL_SLOT_MASK_COMPONENT 99 // Using a unique internal ID

// The actual GTA component ID for masks, used in CharacterCreator_Apply()
#define GTA_COMPONENT_MASK_SLOT 1

// --- GTA ONLINE PARENT DATA ---
struct ParentEntry { const char* name; int index; };
static const ParentEntry dads[] = {
    {"Benjamin", 0}, {"Daniel", 1}, {"Joshua", 2}, {"Noah", 3}, {"Andrew", 4},
    {"Juan", 5}, {"Alex", 6}, {"Isaac", 7}, {"Evan", 8}, {"Ethan", 9},
    {"Vincent", 10}, {"Angel", 11}, {"Diego", 12}, {"Adrian", 13}, {"Gabriel", 14},
    {"Michael", 15}, {"Santiago", 16}, {"Kevin", 17}, {"Louis", 18}, {"Samuel", 19},
    {"Anthony", 20}, {"John", 42}, {"Niko", 43}, {"Claude", 44}
};
static const ParentEntry moms[] = {
    {"Hannah", 21}, {"Audrey", 22}, {"Jasmine", 23}, {"Giselle", 24}, {"Amelia", 25},
    {"Isabella", 26}, {"Zoe", 27}, {"Ava", 28}, {"Camila", 29}, {"Violet", 30},
    {"Sophia", 31}, {"Evelyn", 32}, {"Nicole", 33}, {"Ashley", 34}, {"Grace", 35},
    {"Brianna", 36}, {"Natalie", 37}, {"Olivia", 38}, {"Elizabeth", 39}, {"Charlotte", 40},
    {"Emma", 41}, {"Misty", 45}
};
static const int NUM_DADS = sizeof(dads) / sizeof(dads[0]);
static const int NUM_MOMS = sizeof(moms) / sizeof(moms[0]);
static const int NUM_FACE_FEATURES = 20;
static const char* FACE_FEATURE_NAMES[NUM_FACE_FEATURES] = {
    "Nose Width", "Nose Peak", "Nose Length", "Nose Curve", "Nose Tip",
    "Nose Twist", "Brow Height", "Brow Forward", "Cheekbone Height", "Cheekbone Width",
    "Cheek Width", "Eye Opening", "Lip Thickness", "Jaw Width", "Jaw Shape",
    "Chin Bone", "Chin Length", "Chin Shape", "Chin Hole", "Neck Thickness"
};
static const int NUM_HAIRSTYLES = 74, NUM_HAIRCOLORS = 64, NUM_EYEBROWS = 34, NUM_EYEBROWCOLORS = 64, NUM_EYECOLORS = 32, NUM_TOPS = 350, NUM_UNDERSHIRTS = 150, NUM_LEGS = 120, NUM_SHOES = 100;
static const char* GENDERS[] = { "Male", "Female" };

// Beard customization constants
static const int NUM_BEARDS = 29; // Common number of beard styles
static const int NUM_BEARDCOLORS = 64; // Same as hair colors

// State
static int gender = 0, dadIdx = 0, momIdx = 0;
static int blend = 50, skin = 50;
static int hairIdx = 0, hairColor = 0, eyebrowIdx = 0, eyebrowColor = 0, eyeColor = 0;
// Beard state variables
static int beardIdx = 255, beardColor = 0; // Default 255 to disable beard initially
static float faceFeatures[NUM_FACE_FEATURES] = { 0 };
static int topIdx = 0, undershirtIdx = 0, legIdx = 0, shoeIdx = 0;

// Clothing texture variation state variables
static int topTextureIdx = 0;
static int undershirtTextureIdx = 0;
static int legTextureIdx = 0;
static int shoeTextureIdx = 0;

// Prop variation state variables
static int hatPropIdx = -1; // -1 means no prop
static int hatPropTextureIdx = 0;
static int glassesPropIdx = -1;
static int glassesPropTextureIdx = 0;
static int miscAccessoryIdx = -1; // For SLOT_SPECIAL (component 7)
static int miscAccessoryTextureIdx = 0;
static int maskComponentIdx = -1; // For SLOT_MASK_COMPONENT (component 1)
static int maskComponentTextureIdx = 0;


// Camera state
bool wardrobeCamActive = false;
bool creatorCamEnabled = false;
static Cam customCam = 0;

// --- UI Constants (Defined in script.cpp, declared here as extern) ---
extern const int FONT;
extern const RGBA BG_COLOR;
extern const RGBA HEADER_COLOR;
extern const RGBA TAB_BG_COLOR;
extern const RGBA SELECTED_TAB_COLOR;
extern const RGBA OPTION_COLOR;
extern const RGBA SELECTED_COLOR;
extern const RGBA TEXT_COLOR;
extern const RGBA SELECTED_TEXT_COLOR;
extern const RGBA HEADER_TEXT_COLOR;


// Clamp helper
template<typename T>
static T clamp(T v, T lo, T hi) { return (v < lo) ? lo : (v > hi) ? hi : v; }

// Function to display notifications (declared extern, defined in script.cpp)
extern void ShowNotification(const char* message);


// --- Camera Management ---
void update_character_camera() {
    Ped playerPed = PLAYER::PLAYER_PED_ID();
    if (!ENTITY::DOES_ENTITY_EXIST(playerPed)) return;

    Vector3 headPos = PED::GET_PED_BONE_COORDS(playerPed, 31086, 0, 0, 0); // SKEL_Head

    if (customCam == 0) {
        customCam = CAM::CREATE_CAM("DEFAULT_SCRIPTED_CAMERA", true);
    }

    if (wardrobeCamActive) {
        Vector3 camPos = ENTITY::GET_OFFSET_FROM_ENTITY_IN_WORLD_COORDS(playerPed, 0.0, 2.5, 0.5);
        CAM::SET_CAM_COORD(customCam, camPos.x, camPos.y, camPos.z);
        CAM::POINT_CAM_AT_COORD(customCam, headPos.x, headPos.y, headPos.z);
    }
    else {
        Vector3 camPos = ENTITY::GET_OFFSET_FROM_ENTITY_IN_WORLD_COORDS(playerPed, 0.0, 1.0, 0.5);
        CAM::SET_CAM_COORD(customCam, camPos.x, camPos.y, camPos.z);
        CAM::POINT_CAM_AT_COORD(customCam, headPos.x, headPos.y, headPos.z + 0.1f);
    }

    CAM::SET_CAM_ACTIVE(customCam, true);
    CAM::RENDER_SCRIPT_CAMS(true, false, 0, true, false);
}

void stop_character_camera() {
    if (customCam != 0) {
        CAM::SET_CAM_ACTIVE(customCam, false);
        CAM::DESTROY_CAM(customCam, false);
        customCam = 0;
    }
    CAM::RENDER_SCRIPT_CAMS(false, false, 0, true, false);
}


// PED Application
void CharacterCreator_Apply() {
    Ped ped = PLAYER::PLAYER_PED_ID();
    Hash mpModel = GAMEPLAY::GET_HASH_KEY(gender ? "mp_f_freemode_01" : "mp_m_freemode_01");
    if (ENTITY::GET_ENTITY_MODEL(ped) != mpModel) {
        STREAMING::REQUEST_MODEL(mpModel);
        int timeout = 0;
        while (!STREAMING::HAS_MODEL_LOADED(mpModel) && timeout++ < 500) WAIT(0);
        if (!STREAMING::HAS_MODEL_LOADED(mpModel)) return;
        PLAYER::SET_PLAYER_MODEL(PLAYER::PLAYER_ID(), mpModel);
        WAIT(100);
        ped = PLAYER::PLAYER_PED_ID();
    }
    PED::SET_PED_HEAD_BLEND_DATA(
        ped, dads[dadIdx].index, moms[momIdx].index, 0, dads[dadIdx].index, moms[momIdx].index, 0,
        blend / 100.0f, skin / 100.0f, 0.0f, false
    );
    PED::SET_PED_COMPONENT_VARIATION(ped, 2, hairIdx, 0, 2);
    PED::_SET_PED_HAIR_COLOR(ped, hairColor, 0);
    PED::SET_PED_HEAD_OVERLAY(ped, 2, eyebrowIdx, 1.0f); // Corrected overlayID for Eyebrows
    PED::_SET_PED_HEAD_OVERLAY_COLOR(ped, 2, 1, eyebrowColor, 0); // Corrected overlayID for Eyebrows Color

    // Apply beard style and color (OverlayID 1 for Facial Hair/Beards)
    PED::SET_PED_HEAD_OVERLAY(ped, 1, beardIdx, 1.0f);
    PED::_SET_PED_HEAD_OVERLAY_COLOR(ped, 1, 1, beardColor, 0);

    PED::_SET_PED_EYE_COLOR(ped, eyeColor);
    for (int i = 0; i < NUM_FACE_FEATURES; ++i)
        PED::_SET_PED_FACE_FEATURE(ped, i, faceFeatures[i]);

    // Apply clothing with texture variations
    PED::SET_PED_COMPONENT_VARIATION(ped, SLOT_TOP, topIdx, topTextureIdx, 2);
    PED::SET_PED_COMPONENT_VARIATION(ped, SLOT_UNDERSHIRT, undershirtIdx, undershirtTextureIdx, 2);
    PED::SET_PED_COMPONENT_VARIATION(ped, SLOT_LEGS, legIdx, legTextureIdx, 2);
    PED::SET_PED_COMPONENT_VARIATION(ped, SLOT_SHOES, shoeIdx, shoeTextureIdx, 2);

    // Regarding Torso (ID 3):
    // The game's clothing system often implicitly updates component 3 (Torso)
    // when component variations like SLOT_TOP (11) or SLOT_UNDERSHIRT (8) are changed.
    // Explicitly setting component 3 can lead to glitches or conflicts if not
    // precisely matched to the current top/undershirt.
    // For now, we rely on the game's default behavior, and a direct
    // SET_PED_COMPONENT_VARIATION for component 3 is not added without
    // a more robust way to determine its correct drawable/texture for every outfit.

    // Apply props
    if (hatPropIdx == -1) {
        PED::CLEAR_PED_PROP(ped, PROP_SLOT_HAT);
    }
    else {
        PED::SET_PED_PROP_INDEX(ped, PROP_SLOT_HAT, hatPropIdx, hatPropTextureIdx, false);
    }

    if (glassesPropIdx == -1) {
        PED::CLEAR_PED_PROP(ped, PROP_SLOT_GLASSES);
    }
    else {
        PED::SET_PED_PROP_INDEX(ped, PROP_SLOT_GLASSES, glassesPropIdx, glassesPropTextureIdx, false);
    }

    // Apply Misc. Accessory (SLOT_SPECIAL component)
    if (miscAccessoryIdx == -1) {
        PED::SET_PED_COMPONENT_VARIATION(ped, SLOT_SPECIAL, 0, 0, 2); // Set to default/empty if no accessory
    }
    else {
        PED::SET_PED_COMPONENT_VARIATION(ped, SLOT_SPECIAL, miscAccessoryIdx, miscAccessoryTextureIdx, 2);
    }

    // Apply Mask (GTA_COMPONENT_MASK_SLOT component)
    if (maskComponentIdx == -1) {
        PED::SET_PED_COMPONENT_VARIATION(ped, GTA_COMPONENT_MASK_SLOT, 0, 0, 2); // Set to default/empty if no mask
    }
    else {
        PED::SET_PED_COMPONENT_VARIATION(ped, GTA_COMPONENT_MASK_SLOT, maskComponentIdx, maskComponentTextureIdx, 2);
    }
}

void CharacterCreator_Init() {}
void CharacterCreator_Tick() {}

void CharacterCreator_Save(const char* path) {
    FILE* f = fopen(path, "w");
    if (!f) return;
    fprintf(f, "gender=%d\ndad=%d\nmom=%d\nblend=%d\nskin=%d\nhair=%d\nhairColor=%d\neyebrow=%d\neyebrowColor=%d\neyeColor=%d\n",
        gender, dadIdx, momIdx, blend, skin, hairIdx, hairColor, eyebrowIdx, eyebrowColor, eyeColor);
    // ADDED: Save beard data
    fprintf(f, "beard=%d\nbeardColor=%d\n", beardIdx, beardColor);
    for (int i = 0; i < NUM_FACE_FEATURES; ++i) fprintf(f, "face%d=%.3f\n", i, faceFeatures[i]);
    fprintf(f, "top=%d\nundershirt=%d\nlegs=%d\nshoes=%d\n", topIdx, undershirtIdx, legIdx, shoeIdx);
    // Save new texture indices
    fprintf(f, "topTexture=%d\nundershirtTexture=%d\nlegsTexture=%d\nshoesTexture=%d\n",
        topTextureIdx, undershirtTextureIdx, legTextureIdx, shoeTextureIdx);
    // Save prop indices
    fprintf(f, "hatProp=%d\nhatPropTexture=%d\nglassesProp=%d\nglassesPropTexture=%d\nmiscAccessory=%d\nmiscAccessoryTexture=%d\nmaskComponent=%d\nmaskComponentTexture=%d\n",
        hatPropIdx, hatPropTextureIdx, glassesPropIdx, glassesPropTextureIdx, miscAccessoryIdx, miscAccessoryTextureIdx, maskComponentIdx, maskComponentTextureIdx);
    fclose(f);
}

void CharacterCreator_Load(const char* path) {
    FILE* f = fopen(path, "r");
    if (!f) return;
    char line[128];
    while (fgets(line, sizeof(line), f)) {
        char* eq = strchr(line, '='); if (!eq) continue; *eq = 0; char* val = eq + 1;
        if (strcmp(line, "gender") == 0) gender = atoi(val);
        else if (strcmp(line, "dad") == 0) dadIdx = atoi(val);
        else if (strcmp(line, "mom") == 0) momIdx = atoi(val);
        else if (strcmp(line, "blend") == 0) blend = atoi(val);
        else if (strcmp(line, "skin") == 0) skin = atoi(val);
        else if (strcmp(line, "hair") == 0) hairIdx = atoi(val);
        else if (strcmp(line, "hairColor") == 0) hairColor = atoi(val);
        else if (strcmp(line, "eyebrow") == 0) eyebrowIdx = atoi(val);
        else if (strcmp(line, "eyebrowColor") == 0) eyebrowColor = atoi(val);
        else if (strcmp(line, "eyeColor") == 0) eyeColor = atoi(val);
        // ADDED: Load beard data
        else if (strcmp(line, "beard") == 0) beardIdx = atoi(val);
        else if (strcmp(line, "beardColor") == 0) beardColor = atoi(val);
        else if (strncmp(line, "face", 4) == 0 && isdigit(line[4])) {
            int idx = atoi(line + 4);
            if (idx >= 0 && idx < NUM_FACE_FEATURES) faceFeatures[idx] = (float)atof(val);
        }
        else if (strcmp(line, "top") == 0) topIdx = atoi(val);
        else if (strcmp(line, "undershirt") == 0) undershirtIdx = atoi(val);
        else if (strcmp(line, "legs") == 0) legIdx = atoi(val);
        else if (strcmp(line, "shoes") == 0) shoeIdx = atoi(val);
        // Load new texture indices
        else if (strcmp(line, "topTexture") == 0) topTextureIdx = atoi(val);
        else if (strcmp(line, "undershirtTexture") == 0) undershirtTextureIdx = atoi(val);
        else if (strcmp(line, "legsTexture") == 0) legTextureIdx = atoi(val);
        else if (strcmp(line, "shoesTexture") == 0) shoeTextureIdx = atoi(val);
        // Load prop indices
        else if (strcmp(line, "hatProp") == 0) hatPropIdx = atoi(val);
        else if (strcmp(line, "hatPropTexture") == 0) hatPropTextureIdx = atoi(val);
        else if (strcmp(line, "glassesProp") == 0) glassesPropIdx = atoi(val);
        else if (strcmp(line, "glassesPropTexture") == 0) glassesPropTextureIdx = atoi(val);
        else if (strcmp(line, "miscAccessory") == 0) miscAccessoryIdx = atoi(val); // Changed from mask to miscAccessory
        else if (strcmp(line, "miscAccessoryTexture") == 0) miscAccessoryTextureIdx = atoi(val); // Changed from maskTexture to miscAccessoryTexture
        else if (strcmp(line, "maskComponent") == 0) maskComponentIdx = atoi(val);
        else if (strcmp(line, "maskComponentTexture") == 0) maskComponentTextureIdx = atoi(val);
    }
    fclose(f);
    CharacterCreator_Apply();
    UI::_SET_NOTIFICATION_TEXT_ENTRY("STRING");
    UI::_ADD_TEXT_COMPONENT_STRING("Character Loaded!");
    UI::_DRAW_NOTIFICATION(false, false);
}

// --- Modification ---
void CharacterCreator_ToggleGender() { gender = 1 - gender; CharacterCreator_Apply(); }
void CharacterCreator_NextDad(int dir) { dadIdx = (dadIdx + dir + NUM_DADS) % NUM_DADS; CharacterCreator_Apply(); }
void CharacterCreator_NextMom(int dir) { momIdx = (momIdx + dir + NUM_MOMS) % NUM_MOMS; CharacterCreator_Apply(); }
void CharacterCreator_AdjustShapeBlend(int delta) { blend = clamp(blend + delta, 0, 100); CharacterCreator_Apply(); }
void CharacterCreator_AdjustSkinBlend(int delta) { skin = clamp(skin + delta, 0, 100); CharacterCreator_Apply(); }
void CharacterCreator_NextHairStyle(int dir) { hairIdx = (hairIdx + dir + NUM_HAIRSTYLES) % NUM_HAIRSTYLES; CharacterCreator_Apply(); }
void CharacterCreator_NextHairColor(int dir) { hairColor = (hairColor + dir + NUM_HAIRCOLORS) % NUM_HAIRCOLORS; CharacterCreator_Apply(); }
void CharacterCreator_NextEyebrow(int dir) { eyebrowIdx = (eyebrowIdx + dir + NUM_EYEBROWS) % NUM_EYEBROWS; CharacterCreator_Apply(); }
void CharacterCreator_NextEyebrowColor(int dir) { eyebrowColor = (eyebrowColor + dir + NUM_EYEBROWCOLORS) % NUM_EYEBROWCOLORS; CharacterCreator_Apply(); }

// Beard modification functions
void CharacterCreator_NextBeard(int dir) {
    if (beardIdx == 255 && dir == -1) { // If currently disabled and going backward, go to last beard
        beardIdx = NUM_BEARDS - 1;
    }
    else if (beardIdx == NUM_BEARDS - 1 && dir == 1) { // If last beard and going forward, disable
        beardIdx = 255;
    }
    else if (beardIdx == 255 && dir == 1) { // If currently disabled and going forward, go to first beard
        beardIdx = 0;
    }
    else {
        beardIdx = (beardIdx + dir + NUM_BEARDS) % NUM_BEARDS;
    }
    CharacterCreator_Apply();
}

void CharacterCreator_NextBeardColor(int dir) {
    beardColor = (beardColor + dir + NUM_BEARDCOLORS) % NUM_BEARDCOLORS; CharacterCreator_Apply();
}

void CharacterCreator_NudgeFaceFeature(int idx, float delta) {
    if (idx < 0 || idx >= NUM_FACE_FEATURES) return;
    faceFeatures[idx] = clamp(faceFeatures[idx] + delta, -1.0f, 1.0f);
    CharacterCreator_Apply();
}

void CharacterCreator_NextClothes(int slot, int dir) {
    Ped playerPed = PLAYER::PLAYER_PED_ID();
    int* currentIdx = nullptr;
    int* currentTextureIdx = nullptr;
    int maxDrawables = 0;

    if (slot == 0) { // Top
        currentIdx = &topIdx;
        currentTextureIdx = &topTextureIdx;
        maxDrawables = PED::GET_NUMBER_OF_PED_DRAWABLE_VARIATIONS(playerPed, SLOT_TOP);
    }
    else if (slot == 1) { // Undershirt
        currentIdx = &undershirtIdx;
        currentTextureIdx = &undershirtTextureIdx;
        maxDrawables = PED::GET_NUMBER_OF_PED_DRAWABLE_VARIATIONS(playerPed, SLOT_UNDERSHIRT);
    }
    else if (slot == 2) { // Legs
        currentIdx = &legIdx;
        currentTextureIdx = &legTextureIdx;
        maxDrawables = PED::GET_NUMBER_OF_PED_DRAWABLE_VARIATIONS(playerPed, SLOT_LEGS);
    }
    else if (slot == 3) { // Shoes
        currentIdx = &shoeIdx;
        currentTextureIdx = &shoeTextureIdx;
        maxDrawables = PED::GET_NUMBER_OF_PED_DRAWABLE_VARIATIONS(playerPed, SLOT_SHOES);
    }

    if (currentIdx != nullptr) {
        *currentIdx = (*currentIdx + dir);
        if (*currentIdx < 0) *currentIdx = maxDrawables - 1;
        else if (*currentIdx >= maxDrawables) *currentIdx = 0;

        // Reset texture index if drawable changed
        int maxTextures = PED::GET_NUMBER_OF_PED_TEXTURE_VARIATIONS(playerPed, slot, *currentIdx);
        if (*currentTextureIdx >= maxTextures) {
            *currentTextureIdx = 0;
        }
    }
    CharacterCreator_Apply();
}

void CharacterCreator_NextClothesTexture(int slot, int dir) {
    Ped playerPed = PLAYER::PLAYER_PED_ID();
    int currentDrawable = 0;
    int* currentTextureIdx = nullptr;
    int maxTextures = 0;

    if (slot == 0) { // Top Texture
        currentDrawable = topIdx;
        currentTextureIdx = &topTextureIdx;
        maxTextures = PED::GET_NUMBER_OF_PED_TEXTURE_VARIATIONS(playerPed, SLOT_TOP, currentDrawable);
    }
    else if (slot == 1) { // Undershirt Texture
        currentDrawable = undershirtIdx;
        currentTextureIdx = &undershirtTextureIdx;
        maxTextures = PED::GET_NUMBER_OF_PED_TEXTURE_VARIATIONS(playerPed, SLOT_UNDERSHIRT, currentDrawable);
    }
    else if (slot == 2) { // Legs Texture
        currentDrawable = legIdx;
        currentTextureIdx = &legTextureIdx;
        maxTextures = PED::GET_NUMBER_OF_PED_TEXTURE_VARIATIONS(playerPed, SLOT_LEGS, currentDrawable);
    }
    else if (slot == 3) { // Shoes Texture
        currentDrawable = shoeIdx;
        currentTextureIdx = &shoeTextureIdx;
        maxTextures = PED::GET_NUMBER_OF_PED_TEXTURE_VARIATIONS(playerPed, SLOT_SHOES, currentDrawable);
    }

    if (currentTextureIdx != nullptr && maxTextures > 0) {
        *currentTextureIdx = (*currentTextureIdx + dir);
        if (*currentTextureIdx < 0) *currentTextureIdx = maxTextures - 1;
        else if (*currentTextureIdx >= maxTextures) *currentTextureIdx = 0;
    }
    else if (currentTextureIdx != nullptr) {
        *currentTextureIdx = 0; // No textures available, default to 0
    }
    CharacterCreator_Apply();
}

// New functions for props
void CharacterCreator_NextProp(int propSlot, int dir) {
    Ped playerPed = PLAYER::PLAYER_PED_ID();
    int* currentIdx = nullptr; // Use generic name for drawable/prop index
    int* currentTextureIdx = nullptr;
    int maxVariations = 0; // Use generic name for max drawables/props

    if (propSlot == PROP_SLOT_HAT) {
        currentIdx = &hatPropIdx;
        currentTextureIdx = &hatPropTextureIdx;
        maxVariations = PED::GET_NUMBER_OF_PED_PROP_DRAWABLE_VARIATIONS(playerPed, PROP_SLOT_HAT);
    }
    else if (propSlot == PROP_SLOT_GLASSES) {
        currentIdx = &glassesPropIdx;
        currentTextureIdx = &glassesPropTextureIdx;
        maxVariations = PED::GET_NUMBER_OF_PED_PROP_DRAWABLE_VARIATIONS(playerPed, PROP_SLOT_GLASSES);
    }
    else if (propSlot == SLOT_SPECIAL) { // Misc. Accessory (Component slot)
        currentIdx = &miscAccessoryIdx;
        currentTextureIdx = &miscAccessoryTextureIdx;
        maxVariations = PED::GET_NUMBER_OF_PED_DRAWABLE_VARIATIONS(playerPed, SLOT_SPECIAL);
    }
    else if (propSlot == INTERNAL_SLOT_MASK_COMPONENT) { // Mask (Component slot - using internal ID)
        currentIdx = &maskComponentIdx;
        currentTextureIdx = &maskComponentTextureIdx;
        maxVariations = PED::GET_NUMBER_OF_PED_DRAWABLE_VARIATIONS(playerPed, GTA_COMPONENT_MASK_SLOT); // Use actual GTA component ID
    }

    if (currentIdx != nullptr) {
        // Handle -1 (no prop/component) option
        int tempIdx = *currentIdx;
        if (dir > 0) { // Cycle forward
            tempIdx++;
            if (tempIdx > maxVariations - 1) { // If it goes past the last, loop to -1 (None)
                tempIdx = -1;
            }
        }
        else { // Cycle backward
            tempIdx--;
            if (tempIdx < -1) { // If it goes before -1, loop to the last
                tempIdx = maxVariations - 1;
            }
        }
        *currentIdx = tempIdx;

        // Reset texture index if drawable changed or if "None" is selected
        if (*currentIdx == -1) {
            *currentTextureIdx = 0;
        }
        else {
            int currentDrawableForTexture = *currentIdx; // For props and components, the drawable index is the current index
            int maxTextures = 0;
            if (propSlot == SLOT_SPECIAL) { // Misc. Accessory
                maxTextures = PED::GET_NUMBER_OF_PED_TEXTURE_VARIATIONS(playerPed, SLOT_SPECIAL, currentDrawableForTexture);
            }
            else if (propSlot == INTERNAL_SLOT_MASK_COMPONENT) { // Mask
                maxTextures = PED::GET_NUMBER_OF_PED_TEXTURE_VARIATIONS(playerPed, GTA_COMPONENT_MASK_SLOT, currentDrawableForTexture); // Use actual GTA component ID
            }
            else { // It's a prop slot
                maxTextures = PED::GET_NUMBER_OF_PED_PROP_TEXTURE_VARIATIONS(playerPed, propSlot, currentDrawableForTexture);
            }

            if (*currentTextureIdx >= maxTextures) {
                *currentTextureIdx = 0;
            }
        }
    }
    CharacterCreator_Apply();
}

void CharacterCreator_NextPropTexture(int propSlot, int dir) {
    Ped playerPed = PLAYER::PLAYER_PED_ID();
    int currentDrawableIdx = 0;
    int* currentTextureIdx = nullptr;
    int maxTextures = 0;

    if (propSlot == PROP_SLOT_HAT) {
        currentDrawableIdx = hatPropIdx;
        currentTextureIdx = &hatPropTextureIdx;
    }
    else if (propSlot == PROP_SLOT_GLASSES) {
        currentDrawableIdx = glassesPropIdx;
        currentTextureIdx = &glassesPropTextureIdx;
    }
    else if (propSlot == SLOT_SPECIAL) { // Misc. Accessory
        currentDrawableIdx = miscAccessoryIdx;
        currentTextureIdx = &miscAccessoryTextureIdx;
    }
    else if (propSlot == INTERNAL_SLOT_MASK_COMPONENT) { // Mask
        currentDrawableIdx = maskComponentIdx;
        currentTextureIdx = &maskComponentTextureIdx;
    }

    // Only proceed if a drawable is actually selected (not -1) and texture index pointer is valid
    if (currentDrawableIdx != -1 && currentTextureIdx != nullptr) {
        if (propSlot == SLOT_SPECIAL) { // Misc. Accessory
            maxTextures = PED::GET_NUMBER_OF_PED_TEXTURE_VARIATIONS(playerPed, SLOT_SPECIAL, currentDrawableIdx);
        }
        else if (propSlot == INTERNAL_SLOT_MASK_COMPONENT) { // Mask
            maxTextures = PED::GET_NUMBER_OF_PED_TEXTURE_VARIATIONS(playerPed, GTA_COMPONENT_MASK_SLOT, currentDrawableIdx); // Use actual GTA component ID
        }
        else { // It's a prop slot
            maxTextures = PED::GET_NUMBER_OF_PED_PROP_TEXTURE_VARIATIONS(playerPed, propSlot, currentDrawableIdx);
        }

        if (maxTextures > 0) {
            *currentTextureIdx = (*currentTextureIdx + dir);
            if (*currentTextureIdx < 0) *currentTextureIdx = maxTextures - 1;
            else if (*currentTextureIdx >= maxTextures) *currentTextureIdx = 0;
        }
        else {
            *currentTextureIdx = 0; // No textures available, default to 0
        }
    }
    CharacterCreator_Apply();
}


void CharacterCreator_NextEyeColor(int dir) { eyeColor = (eyeColor + dir + NUM_EYECOLORS) % NUM_EYECOLORS; CharacterCreator_Apply(); }

// --- Menu logic with controller support ---
enum CreatorPage { CREATOR_MAIN = 0, CREATOR_FACE, CREATOR_CLOTHES };
static int creatorPage = 0;


// =================================================================================
// --- REVAMPED MENU DRAWING FUNCTION ---
// =================================================================================
void CharacterCreator_DrawMenu(int& menuIndex, int& menuCategory) {
    float x = 0.02f, y = 0.13f, w = 0.29f, h = 0.038f;

    // Determine the number of options for the current page
    int numOptions = 0;
    if (creatorPage == CREATOR_MAIN) {
        numOptions = 15; // Number of main menu options without tattoos
    }
    else if (creatorPage == CREATOR_FACE) {
        numOptions = NUM_FACE_FEATURES + 1; // Face page has 20 features + Back button
    }
    else if (creatorPage == CREATOR_CLOTHES) {
        numOptions = 18; // 4 clothing drawables + 4 texture variations + 2 prop drawables + 2 prop textures + 2 component drawables + 2 component textures + Wardrobe Cam + Back to Main
    }

    // Calculate total height including tab bar and options
    float tabBarHeight = 0.028f; // Height of the tab bar itself
    float optionsContentHeight = h * numOptions; // Total height of all options
    float totalMenuHeight = tabBarHeight + optionsContentHeight + (MENU_H - 0.004f); // Add a little extra space for bottom padding

    // Calculate the center Y for the entire menu block (tabs + options)
    float menuBlockCenterY = y - 0.053f + (totalMenuHeight / 2.0f); // y - 0.053f is the top of the tabs

    // Draw ONE unified background for the tabs and options.
    GRAPHICS::DRAW_RECT(x + w * 0.5f, menuBlockCenterY, w, totalMenuHeight, BG_COLOR.r, BG_COLOR.g, BG_COLOR.b, BG_COLOR.a);


    // --- Page Tab Drawing & Navigation ---
    const char* pages[3] = { "Main", "Face", "Clothes" };
    float tabRowY = y - 0.053f; // Y position for the top of the tab row
    float tabSectionWidth = w / 3.0f; // Each tab takes 1/3 of the menu width

    for (int i = 0; i < 3; ++i) {
        float tabX = x + (tabSectionWidth * i) + (tabSectionWidth / 2.0f); // Center X for each tab
        RGBA currentTabColor = (i == creatorPage) ? SELECTED_TAB_COLOR : TAB_BG_COLOR;

        // Draw the tab background
        GRAPHICS::DRAW_RECT(tabX, tabRowY + (tabBarHeight / 2.0f), tabSectionWidth, tabBarHeight,
            currentTabColor.r, currentTabColor.g, currentTabColor.b, currentTabColor.a);

        // Draw the tab text
        UI::SET_TEXT_FONT(FONT);
        UI::SET_TEXT_SCALE(0.0f, 0.36f);
        UI::SET_TEXT_COLOUR(TEXT_COLOR.r, TEXT_COLOR.g, TEXT_COLOR.b, TEXT_COLOR.a);
        UI::SET_TEXT_CENTRE(1);
        UI::_SET_TEXT_ENTRY("STRING");
        UI::_ADD_TEXT_COMPONENT_STRING(const_cast<char*>(pages[i]));
        UI::_DRAW_TEXT(tabX, tabRowY + 0.005f); // Position text within the tab
    }
    UI::SET_TEXT_CENTRE(0); // Reset text alignment after drawing tabs

    // Handle Tab Switching Input
    extern int inputDelayFrames; // Access global input delay
    if (inputDelayFrames == 0) { // Only process tab switching if no global input delay
        if (IsKeyJustUp(VK_NUMPAD7) || IsKeyJustUp(VK_OEM_4) || PadPressed(BTN_LB)) { //
            creatorPage = (creatorPage + 2) % 3; menuIndex = 0;
            inputDelayFrames = 15; // Increased delay from 10 to 15
        }
        if (IsKeyJustUp(VK_NUMPAD9) || IsKeyJustUp(VK_OEM_6) || PadPressed(BTN_RB)) { //
            creatorPage = (creatorPage + 1) % 3; menuIndex = 0;
            inputDelayFrames = 15; // Increased delay from 10 to 15
        }
    }


    // --- Page-Specific Options Drawing ---
    float optionsStartY = y; // Options start at MENU_Y, directly below the tab bar visually

    if (creatorPage == CREATOR_MAIN) {
        wardrobeCamActive = false; // Ensure wardrobe cam is off on this page
        const int numMainOptions = 15; // Number of options in the main Character Creator menu
        const char* labels[numMainOptions] = {
            "Creator Camera", "Gender","Dad","Mom","Shape Blend","Skin Blend","Hair Style","Hair Color",
            "Eyebrows","Eyebrow Color","Beard Style","Beard Color","Eye Color","Face Features","Clothes"
        };

        for (int i = 0; i < numMainOptions; ++i) {
            char val[64] = {};
            if (i == 0) sprintf_s(val, "%s", creatorCamEnabled ? "[ON]" : "[OFF]");
            else if (i == 1) sprintf_s(val, "< %s >", GENDERS[gender]);
            else if (i == 2) sprintf_s(val, "< %s >", dads[dadIdx].name);
            else if (i == 3) sprintf_s(val, "< %s >", moms[momIdx].name);
            else if (i == 4) sprintf_s(val, "< %d%% >", blend);
            else if (i == 5) sprintf_s(val, "< %d%% >", skin);
            else if (i == 6) sprintf_s(val, "< %d >", hairIdx);
            else if (i == 7) sprintf_s(val, "< %d >", hairColor);
            else if (i == 8) sprintf_s(val, "< %d >", eyebrowIdx);
            else if (i == 9) sprintf_s(val, "< %d >", eyebrowColor);
            // Display for Beard Style and Color (Corrected format for beardIdx)
            else if (i == 10) { // Beard Style
                if (beardIdx == 255) sprintf_s(val, "< None >");
                else sprintf_s(val, "< %d >", beardIdx);
            }
            else if (i == 11) sprintf_s(val, "< %d >", beardColor);
            else if (i == 12) sprintf_s(val, "< %d >", eyeColor);


            if (i < 13) DrawPairedMenuOption(labels[i], val, x, optionsStartY + h * i, w, h, i == menuIndex);
            else DrawMenuOption(labels[i], x, optionsStartY + h * i, w, h, i == menuIndex);
        }

        int up = 0, down = 0, left = 0, right = 0;
        if (inputDelayFrames == 0) { // Apply input delay to main menu navigation
            if (IsKeyJustUp(VK_NUMPAD8) || IsKeyJustUp(VK_UP) || PadPressed(DPAD_UP)) up = 1; //
            if (IsKeyJustUp(VK_NUMPAD2) || IsKeyJustUp(VK_DOWN) || PadPressed(DPAD_DOWN)) down = 1; //
            if (IsKeyJustUp(VK_NUMPAD4) || IsKeyJustUp(VK_LEFT) || PadPressed(DPAD_LEFT)) left = 1; //
            if (IsKeyJustUp(VK_NUMPAD6) || IsKeyJustUp(VK_RIGHT) || PadPressed(DPAD_RIGHT)) right = 1; //

            if (up) { menuIndex = (menuIndex - 1 + numMainOptions) % numMainOptions; inputDelayFrames = 10; }
            if (down) { menuIndex = (menuIndex + 1) % numMainOptions; inputDelayFrames = 10; }

            int dir = right - left;
            if (dir) {
                switch (menuIndex) {
                case 1: CharacterCreator_ToggleGender(); inputDelayFrames = 10; break;
                case 2: CharacterCreator_NextDad(dir); inputDelayFrames = 10; break;
                case 3: CharacterCreator_NextMom(dir); inputDelayFrames = 10; break;
                case 4: CharacterCreator_AdjustShapeBlend(dir); inputDelayFrames = 10; break;
                case 5: CharacterCreator_AdjustSkinBlend(dir); inputDelayFrames = 10; break;
                case 6: CharacterCreator_NextHairStyle(dir); inputDelayFrames = 10; break;
                case 7: CharacterCreator_NextHairColor(dir); inputDelayFrames = 10; break;
                case 8: CharacterCreator_NextEyebrow(dir); inputDelayFrames = 10; break;
                case 9: CharacterCreator_NextEyebrowColor(dir); inputDelayFrames = 10; break;
                    // Beard Style and Color
                case 10: CharacterCreator_NextBeard(dir); inputDelayFrames = 10; break;
                case 11: CharacterCreator_NextBeardColor(dir); inputDelayFrames = 10; break;
                case 12: CharacterCreator_NextEyeColor(dir); inputDelayFrames = 10; break;
                }
            }
            if (IsKeyJustUp(VK_NUMPAD5) || IsKeyJustUp(VK_RETURN) || PadPressed(BTN_A)) { //
                if (menuIndex == 0) { creatorCamEnabled = !creatorCamEnabled; inputDelayFrames = 10; }
                if (menuIndex == 13) { creatorPage = CREATOR_FACE; menuIndex = 0; inputDelayFrames = 10; }
                if (menuIndex == 14) { creatorPage = CREATOR_CLOTHES; menuIndex = 0; inputDelayFrames = 10; }
            }
        }
        if (PadPressed(BTN_B) || IsKeyJustUp(VK_ESCAPE) || IsKeyJustUp(VK_BACK)) { //
            creatorCamEnabled = false;
            wardrobeCamActive = false;
            stop_character_camera();
            menuCategory = 0;
            menuIndex = 0;
            creatorPage = 0;
        }
    }
    else if (creatorPage == CREATOR_FACE) {
        wardrobeCamActive = false; // Ensure wardrobe cam is off on this page
        const int numFaceOptions = NUM_FACE_FEATURES + 1; // 20 features + Back button

        for (int i = 0; i < numFaceOptions; ++i) {
            if (i < NUM_FACE_FEATURES) {
                char vbuf[32]; sprintf_s(vbuf, "< %.2f >", faceFeatures[i]);
                DrawPairedMenuOption(FACE_FEATURE_NAMES[i], vbuf, x, optionsStartY + h * i, w, h, i == menuIndex);
            }
            else {
                DrawMenuOption("Back to Main", x, optionsStartY + h * i, w, h, i == menuIndex);
            }
        }

        int up = 0, down = 0, left = 0, right = 0;
        if (inputDelayFrames == 0) { // Apply input delay to face menu navigation
            if (IsKeyJustUp(VK_NUMPAD8) || IsKeyJustUp(VK_UP) || PadPressed(DPAD_UP)) up = 1; //
            if (IsKeyJustUp(VK_NUMPAD2) || IsKeyJustUp(VK_DOWN) || PadPressed(DPAD_DOWN)) down = 1; //
            if (IsKeyJustUp(VK_NUMPAD4) || IsKeyJustUp(VK_LEFT) || PadPressed(DPAD_LEFT)) left = 1; //
            if (IsKeyJustUp(VK_NUMPAD6) || IsKeyJustUp(VK_RIGHT) || PadPressed(DPAD_RIGHT)) right = 1; //

            if (up) { menuIndex = (menuIndex - 1 + numFaceOptions) % numFaceOptions; inputDelayFrames = 10; }
            if (down) { menuIndex = (menuIndex + 1) % numFaceOptions; inputDelayFrames = 10; }

            int dir = right - left;
            if (dir && menuIndex < NUM_FACE_FEATURES) { CharacterCreator_NudgeFaceFeature(menuIndex, dir * 0.05f); inputDelayFrames = 10; } // Face Feature Nudge
            if (IsKeyJustUp(VK_NUMPAD5) || IsKeyJustUp(VK_RETURN) || PadPressed(BTN_A)) { //
                if (menuIndex == numFaceOptions - 1) { creatorPage = CREATOR_MAIN; menuIndex = 0; inputDelayFrames = 10; }
            }
        }
        if (PadPressed(BTN_B) || IsKeyJustUp(VK_ESCAPE) || IsKeyJustUp(VK_BACK)) { creatorPage = CREATOR_MAIN; menuIndex = 0; inputDelayFrames = 10; } //
    }
    else if (creatorPage == CREATOR_CLOTHES) {
        // Updated numClothesOptions for new mask slot and misc accessory
        const int numClothesOptions = 18; // 4 clothing drawables + 4 texture variations + 2 prop drawables + 2 prop textures + 2 component drawables + 2 component textures + Wardrobe Cam + Back to Main
        const char* labels[numClothesOptions] = {
            "Top", "Top Texture", "Undershirt", "Undershirt Texture",
            "Legs", "Legs Texture", "Shoes", "Shoes Texture",
            "Hat", "Hat Texture", "Glasses", "Glasses Texture",
            "Misc. Accessory", "Misc. Accessory Texture", // Renamed from Mask
            "Mask", "Mask Texture", // New dedicated Mask slot
            "Wardrobe Cam", "Back to Main"
        };

        for (int i = 0; i < numClothesOptions; ++i) {
            char vbuf[32];
            if (i == 0) sprintf_s(vbuf, "< %d >", topIdx);
            else if (i == 1) sprintf_s(vbuf, "< %d >", topTextureIdx);
            else if (i == 2) sprintf_s(vbuf, "< %d >", undershirtIdx);
            else if (i == 3) sprintf_s(vbuf, "< %d >", undershirtTextureIdx);
            else if (i == 4) sprintf_s(vbuf, "< %d >", legIdx);
            else if (i == 5) sprintf_s(vbuf, "< %d >", legTextureIdx);
            else if (i == 6) sprintf_s(vbuf, "< %d >", shoeIdx);
            else if (i == 7) sprintf_s(vbuf, "< %d >", shoeTextureIdx);
            else if (i == 8) { // Hat
                if (hatPropIdx == -1) sprintf_s(vbuf, "< None >");
                else sprintf_s(vbuf, "< %d >", hatPropIdx);
            }
            else if (i == 9) sprintf_s(vbuf, "< %d >", hatPropTextureIdx);
            else if (i == 10) { // Glasses
                if (glassesPropIdx == -1) sprintf_s(vbuf, "< None >");
                else sprintf_s(vbuf, "< %d >", glassesPropIdx);
            }
            else if (i == 11) sprintf_s(vbuf, "< %d >", glassesPropTextureIdx);
            else if (i == 12) { // Misc. Accessory
                if (miscAccessoryIdx == -1) sprintf_s(vbuf, "< None >");
                else sprintf_s(vbuf, "< %d >", miscAccessoryIdx);
            }
            else if (i == 13) sprintf_s(vbuf, "< %d >", miscAccessoryTextureIdx);
            else if (i == 14) { // Mask (new dedicated slot)
                if (maskComponentIdx == -1) sprintf_s(vbuf, "< None >");
                else sprintf_s(vbuf, "< %d >", maskComponentIdx);
            }
            else if (i == 15) sprintf_s(vbuf, "< %d >", maskComponentTextureIdx);
            else if (i == 16) sprintf_s(vbuf, "%s", wardrobeCamActive ? "[ON]" : "[OFF]");

            DrawPairedMenuOption(labels[i], vbuf, x, optionsStartY + h * i, w, h, i == menuIndex);
        }

        int up = 0, down = 0, left = 0, right = 0;
        if (inputDelayFrames == 0) { // Apply input delay to clothes menu navigation
            if (IsKeyJustUp(VK_NUMPAD8) || IsKeyJustUp(VK_UP) || PadPressed(DPAD_UP)) up = 1; //
            if (IsKeyJustUp(VK_NUMPAD2) || IsKeyJustUp(VK_DOWN) || PadPressed(DPAD_DOWN)) down = 1; //
            if (IsKeyJustUp(VK_NUMPAD4) || IsKeyJustUp(VK_LEFT) || PadPressed(DPAD_LEFT)) left = 1; //
            if (IsKeyJustUp(VK_NUMPAD6) || IsKeyJustUp(VK_RIGHT) || PadPressed(DPAD_RIGHT)) right = 1; //

            if (up) { menuIndex = (menuIndex - 1 + numClothesOptions) % numClothesOptions; inputDelayFrames = 10; }
            if (down) { menuIndex = (menuIndex + 1) % numClothesOptions; inputDelayFrames = 10; }

            int dir = right - left;
            if (dir) {
                switch (menuIndex) {
                case 0: CharacterCreator_NextClothes(0, dir); inputDelayFrames = 10; break; // Top Drawable
                case 1: CharacterCreator_NextClothesTexture(0, dir); inputDelayFrames = 10; break; // Top Texture
                case 2: CharacterCreator_NextClothes(1, dir); inputDelayFrames = 10; break; // Undershirt Drawable
                case 3: CharacterCreator_NextClothesTexture(1, dir); inputDelayFrames = 10; break; // Undershirt Texture
                case 4: CharacterCreator_NextClothes(2, dir); inputDelayFrames = 10; break; // Legs Drawable
                case 5: CharacterCreator_NextClothesTexture(2, dir); inputDelayFrames = 10; break; // Legs Texture
                case 6: CharacterCreator_NextClothes(3, dir); inputDelayFrames = 10; break; // Shoes Drawable
                case 7: CharacterCreator_NextClothesTexture(3, dir); inputDelayFrames = 10; break; // Shoes Texture
                case 8: CharacterCreator_NextProp(PROP_SLOT_HAT, dir); inputDelayFrames = 10; break; // Hat Drawable
                case 9: CharacterCreator_NextPropTexture(PROP_SLOT_HAT, dir); inputDelayFrames = 10; break; // Hat Texture
                case 10: CharacterCreator_NextProp(PROP_SLOT_GLASSES, dir); inputDelayFrames = 10; break; // Glasses Drawable
                case 11: CharacterCreator_NextPropTexture(PROP_SLOT_GLASSES, dir); inputDelayFrames = 10; break; // Glasses Texture
                case 12: CharacterCreator_NextMiscAccessory(SLOT_SPECIAL, dir); inputDelayFrames = 10; break; // Misc. Accessory Drawable
                case 13: CharacterCreator_NextMiscAccessoryTexture(SLOT_SPECIAL, dir); inputDelayFrames = 10; break; // Misc. Accessory Texture
                case 14: CharacterCreator_NextMask(INTERNAL_SLOT_MASK_COMPONENT, dir); inputDelayFrames = 10; break; // Mask Drawable (using internal ID)
                case 15: CharacterCreator_NextMaskTexture(INTERNAL_SLOT_MASK_COMPONENT, dir); inputDelayFrames = 10; break; // Mask Texture (using internal ID)
                }
            }

            if (IsKeyJustUp(VK_NUMPAD5) || IsKeyJustUp(VK_RETURN) || PadPressed(BTN_A)) { //
                if (menuIndex == 16) { // Wardrobe Cam
                    wardrobeCamActive = !wardrobeCamActive;
                    creatorCamEnabled = wardrobeCamActive; // Keep creatorCamEnabled in sync
                    inputDelayFrames = 10;
                }
                if (menuIndex == numClothesOptions - 1) { // Back to Main
                    creatorPage = CREATOR_MAIN;
                    menuIndex = 0;
                    inputDelayFrames = 10;
                }
            }
        }
        if (PadPressed(BTN_B) || IsKeyJustUp(VK_ESCAPE) || IsKeyJustUp(VK_BACK)) { creatorPage = CREATOR_MAIN; menuIndex = 0; inputDelayFrames = 10; } //
    }

    if (creatorCamEnabled) {
        update_character_camera();
    }
    else {
        stop_character_camera();
    }
}

// Definitions for new functions to resolve linker errors.
// These functions act as wrappers to the existing generic prop/component handlers.

void CharacterCreator_NextMask(int slot, int dir) {
    // This calls the generic prop handling function, which correctly
    // processes variations for INTERNAL_SLOT_MASK_COMPONENT (which is 'slot' here).
    CharacterCreator_NextProp(slot, dir);
}

void CharacterCreator_NextMaskTexture(int slot, int dir) {
    // This calls the generic prop texture handling function, which correctly
    // processes texture variations for INTERNAL_SLOT_MASK_COMPONENT (which is 'slot' here).
    CharacterCreator_NextPropTexture(slot, dir);
}

void CharacterCreator_NextMiscAccessory(int slot, int dir) {
    // This calls the generic prop handling function, which correctly
    // processes variations for SLOT_SPECIAL (which is 'slot' here).
    CharacterCreator_NextProp(slot, dir);
}

void CharacterCreator_NextMiscAccessoryTexture(int slot, int dir) {
    // This calls the generic prop texture handling function, which correctly
    // processes texture variations for SLOT_SPECIAL (which is 'slot' here).
    CharacterCreator_NextPropTexture(slot, dir);
}