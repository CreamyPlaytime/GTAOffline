#include "CarShop.h"
#include "Money.h"
#include "input.h"
#include "Garage.h" 
#include "script.h" 
#include <vector>
#include <fstream>

// --- Global Variables ---
static std::vector<VehicleCategory> g_vehicleCategories;
static int carCategoryIndex = 0;
static int vehicleIndex = 0;
static bool inVehicleSelection = false;

// --- Initialization ---
void CarShop_Init() {
    // Super Cars
    VehicleCategory super;
    super.name = "Super";
    super.vehicles.push_back({ "Adder", GAMEPLAY::GET_HASH_KEY("ADDER"), 1000000 });
    super.vehicles.push_back({ "T20", GAMEPLAY::GET_HASH_KEY("T20"), 2200000 });
    super.vehicles.push_back({ "Zentorno", GAMEPLAY::GET_HASH_KEY("ZENTORNO"), 725000 });
    super.vehicles.push_back({ "Turismo R", GAMEPLAY::GET_HASH_KEY("TURISMOR"), 500000 });
    super.vehicles.push_back({ "Osiris", GAMEPLAY::GET_HASH_KEY("OSIRIS"), 1950000 });
    super.vehicles.push_back({ "Entity XF", GAMEPLAY::GET_HASH_KEY("ENTITYXF"), 795000 });
    super.vehicles.push_back({ "Cheetah", GAMEPLAY::GET_HASH_KEY("CHEETAH"), 650000 });
    super.vehicles.push_back({ "Infernus", GAMEPLAY::GET_HASH_KEY("INFERNUS"), 440000 });
    super.vehicles.push_back({ "Vacca", GAMEPLAY::GET_HASH_KEY("VACCA"), 240000 });
    super.vehicles.push_back({ "Bullet", GAMEPLAY::GET_HASH_KEY("BULLET"), 155000 });
    g_vehicleCategories.push_back(super);

    // Sports Cars
    VehicleCategory sports;
    sports.name = "Sports";
    sports.vehicles.push_back({ "Elegy RH8", GAMEPLAY::GET_HASH_KEY("ELEGY2"), 95000 });
    sports.vehicles.push_back({ "Massacro", GAMEPLAY::GET_HASH_KEY("MASSACRO"), 275000 });
    sports.vehicles.push_back({ "Jester", GAMEPLAY::GET_HASH_KEY("JESTER"), 240000 });
    sports.vehicles.push_back({ "Feltzer", GAMEPLAY::GET_HASH_KEY("FELTZER2"), 145000 });
    sports.vehicles.push_back({ "Comet", GAMEPLAY::GET_HASH_KEY("COMET2"), 100000 });
    sports.vehicles.push_back({ "Banshee", GAMEPLAY::GET_HASH_KEY("BANSHEE"), 105000 });
    sports.vehicles.push_back({ "Sultan", GAMEPLAY::GET_HASH_KEY("SULTAN"), 12000 });
    sports.vehicles.push_back({ "9F", GAMEPLAY::GET_HASH_KEY("NINEF"), 120000 });
    sports.vehicles.push_back({ "Carbonizzare", GAMEPLAY::GET_HASH_KEY("CARBONIZZARE"), 195000 });
    sports.vehicles.push_back({ "Rapid GT", GAMEPLAY::GET_HASH_KEY("RAPIDGT"), 132000 });
    g_vehicleCategories.push_back(sports);

    // Sports Classics
    VehicleCategory classics;
    classics.name = "Sports Classics";
    classics.vehicles.push_back({ "Z-Type", GAMEPLAY::GET_HASH_KEY("ZTYPE"), 950000 });
    classics.vehicles.push_back({ "Stinger GT", GAMEPLAY::GET_HASH_KEY("STINGERGT"), 875000 });
    classics.vehicles.push_back({ "Monroe", GAMEPLAY::GET_HASH_KEY("MONROE"), 490000 });
    classics.vehicles.push_back({ "Coquette Classic", GAMEPLAY::GET_HASH_KEY("COQUETTE2"), 138000 });
    classics.vehicles.push_back({ "Manana", GAMEPLAY::GET_HASH_KEY("MANANA"), 10000 });
    g_vehicleCategories.push_back(classics);

    // Muscle Cars
    VehicleCategory muscle;
    muscle.name = "Muscle";
    muscle.vehicles.push_back({ "Dominator", GAMEPLAY::GET_HASH_KEY("DOMINATOR"), 35000 });
    muscle.vehicles.push_back({ "Gauntlet", GAMEPLAY::GET_HASH_KEY("GAUNTLET"), 32000 });
    muscle.vehicles.push_back({ "Sabre Turbo", GAMEPLAY::GET_HASH_KEY("SABREGT"), 15000 });
    muscle.vehicles.push_back({ "Ruiner", GAMEPLAY::GET_HASH_KEY("RUINER"), 10000 });
    muscle.vehicles.push_back({ "Vigero", GAMEPLAY::GET_HASH_KEY("VIGERO"), 21000 });
    muscle.vehicles.push_back({ "Phoenix", GAMEPLAY::GET_HASH_KEY("PHOENIX"), 12500 });
    muscle.vehicles.push_back({ "Buccaneer", GAMEPLAY::GET_HASH_KEY("BUCCANEER"), 29000 });
    g_vehicleCategories.push_back(muscle);

    // Sedans
    VehicleCategory sedans;
    sedans.name = "Sedans";
    sedans.vehicles.push_back({ "Oracle", GAMEPLAY::GET_HASH_KEY("ORACLE2"), 80000 });
    sedans.vehicles.push_back({ "Tailgater", GAMEPLAY::GET_HASH_KEY("TAILGATER"), 55000 });
    sedans.vehicles.push_back({ "Schafter", GAMEPLAY::GET_HASH_KEY("SCHAFTER2"), 65000 });
    sedans.vehicles.push_back({ "Super Diamond", GAMEPLAY::GET_HASH_KEY("SUPERD"), 250000 });
    sedans.vehicles.push_back({ "Washington", GAMEPLAY::GET_HASH_KEY("WASHINGTON"), 15000 });
    g_vehicleCategories.push_back(sedans);

    // Coupes
    VehicleCategory coupes;
    coupes.name = "Coupes";
    coupes.vehicles.push_back({ "Felon", GAMEPLAY::GET_HASH_KEY("FELON"), 90000 });
    coupes.vehicles.push_back({ "Zion", GAMEPLAY::GET_HASH_KEY("ZION"), 60000 });
    coupes.vehicles.push_back({ "F620", GAMEPLAY::GET_HASH_KEY("F620"), 80000 });
    coupes.vehicles.push_back({ "Exemplar", GAMEPLAY::GET_HASH_KEY("EXEMPLAR"), 205000 });
    g_vehicleCategories.push_back(coupes);

    // SUVs
    VehicleCategory suvs;
    suvs.name = "SUVs";
    suvs.vehicles.push_back({ "Baller", GAMEPLAY::GET_HASH_KEY("BALLER2"), 90000 });
    suvs.vehicles.push_back({ "Granger", GAMEPLAY::GET_HASH_KEY("GRANGER"), 35000 });
    suvs.vehicles.push_back({ "Dubsta", GAMEPLAY::GET_HASH_KEY("DUBSTA"), 70000 });
    suvs.vehicles.push_back({ "Huntley S", GAMEPLAY::GET_HASH_KEY("HUNTLEY"), 195000 });
    suvs.vehicles.push_back({ "Rocoto", GAMEPLAY::GET_HASH_KEY("ROCOTO"), 85000 });
    g_vehicleCategories.push_back(suvs);

    // Off-Road
    VehicleCategory offroad;
    offroad.name = "Off-Road";
    offroad.vehicles.push_back({ "Sandking XL", GAMEPLAY::GET_HASH_KEY("SANDKING"), 45000 });
    offroad.vehicles.push_back({ "Rebel", GAMEPLAY::GET_HASH_KEY("REBEL2"), 22000 });
    offroad.vehicles.push_back({ "Dune Buggy", GAMEPLAY::GET_HASH_KEY("DUNE"), 20000 });
    offroad.vehicles.push_back({ "Insurgent", GAMEPLAY::GET_HASH_KEY("INSURGENT2"), 675000 });
    offroad.vehicles.push_back({ "Bifta", GAMEPLAY::GET_HASH_KEY("BIFTA"), 75000 });
    offroad.vehicles.push_back({ "Bodhi", GAMEPLAY::GET_HASH_KEY("BODHI2"), 25000 });
    g_vehicleCategories.push_back(offroad);

    // Motorcycles
    VehicleCategory bikes;
    bikes.name = "Motorcycles";
    bikes.vehicles.push_back({ "Akuma", GAMEPLAY::GET_HASH_KEY("AKUMA"), 9000 });
    bikes.vehicles.push_back({ "Bati 801", GAMEPLAY::GET_HASH_KEY("BATI"), 15000 });
    bikes.vehicles.push_back({ "PCJ 600", GAMEPLAY::GET_HASH_KEY("PCJ"), 9000 });
    bikes.vehicles.push_back({ "Sanchez", GAMEPLAY::GET_HASH_KEY("SANCHEZ2"), 8000 });
    bikes.vehicles.push_back({ "Hexer", GAMEPLAY::GET_HASH_KEY("HEXER"), 15000 });
    bikes.vehicles.push_back({ "Double-T", GAMEPLAY::GET_HASH_KEY("DOUBLE"), 12000 });
    g_vehicleCategories.push_back(bikes);

    // Vans
    VehicleCategory vans;
    vans.name = "Vans";
    vans.vehicles.push_back({ "Youga", GAMEPLAY::GET_HASH_KEY("YOUGA"), 16000 });
    vans.vehicles.push_back({ "Rumpo", GAMEPLAY::GET_HASH_KEY("RUMPO"), 13000 });
    vans.vehicles.push_back({ "Bison", GAMEPLAY::GET_HASH_KEY("BISON"), 30000 });
    g_vehicleCategories.push_back(vans);

    // Emergency
    VehicleCategory emergency;
    emergency.name = "Emergency";
    emergency.vehicles.push_back({ "Police Cruiser", GAMEPLAY::GET_HASH_KEY("POLICE"), 50000 });
    emergency.vehicles.push_back({ "Ambulance", GAMEPLAY::GET_HASH_KEY("AMBULANCE"), 55000 });
    emergency.vehicles.push_back({ "Fire Truck", GAMEPLAY::GET_HASH_KEY("FIRETRUK"), 60000 });
    g_vehicleCategories.push_back(emergency);

    // Military
    VehicleCategory military;
    military.name = "Military";
    military.vehicles.push_back({ "Rhino Tank", GAMEPLAY::GET_HASH_KEY("RHINO"), 1500000 });
    military.vehicles.push_back({ "Barracks", GAMEPLAY::GET_HASH_KEY("BARRACKS"), 430000 });
    military.vehicles.push_back({ "Crusader", GAMEPLAY::GET_HASH_KEY("CRUSADER"), 225000 });
    military.vehicles.push_back({ "Lazer", GAMEPLAY::GET_HASH_KEY("LAZER"), 6500000 });
    military.vehicles.push_back({ "Hydra", GAMEPLAY::GET_HASH_KEY("HYDRA"), 3000000 });
    military.vehicles.push_back({ "Valkyrie", GAMEPLAY::GET_HASH_KEY("VALKYRIE"), 2850000 });
    g_vehicleCategories.push_back(military);

    // Boats
    VehicleCategory boats;
    boats.name = "Boats";
    boats.vehicles.push_back({ "Seashark", GAMEPLAY::GET_HASH_KEY("SEASHARK"), 16899 });
    boats.vehicles.push_back({ "Speeder", GAMEPLAY::GET_HASH_KEY("SPEEDER"), 325000 });
    boats.vehicles.push_back({ "Tropic", GAMEPLAY::GET_HASH_KEY("TROPIC"), 60000 });
    g_vehicleCategories.push_back(boats);

    // Planes
    VehicleCategory planes;
    planes.name = "Planes";
    planes.vehicles.push_back({ "Luxor", GAMEPLAY::GET_HASH_KEY("LUXOR"), 1625000 });
    planes.vehicles.push_back({ "Titan", GAMEPLAY::GET_HASH_KEY("TITAN"), 2000000 });
    planes.vehicles.push_back({ "Mallard", GAMEPLAY::GET_HASH_KEY("STUNT"), 250000 });
    g_vehicleCategories.push_back(planes);
}

void CarShop_Tick() {
    // Future logic can go here if needed
}

void draw_car_shop_menu() {
    extern int menuCategory;
    extern int menuIndex;
    extern int inputDelayFrames;

    const float MENU_X = 0.02f;
    const float MENU_Y = 0.13f;
    const float MENU_W = 0.29f;
    const float MENU_H = 0.038f;

    // Display Player's Money at the top
    char moneyBuf[64];
    sprintf_s(moneyBuf, "Your Money: $%d", Money_Get());
    UI::SET_TEXT_FONT(0);
    UI::SET_TEXT_SCALE(0.0f, 0.37f);
    UI::SET_TEXT_COLOUR(130, 255, 130, 220);
    UI::_SET_TEXT_ENTRY("STRING");
    UI::_ADD_TEXT_COMPONENT_STRING(moneyBuf);
    UI::_DRAW_TEXT(MENU_X, MENU_Y - 0.08f);

    if (!inVehicleSelection) {
        // --- Category Selection ---
        const int numOptions = g_vehicleCategories.size() + 1;
        float totalHeight = MENU_H * (numOptions + 1);

        GRAPHICS::DRAW_RECT(MENU_X + MENU_W * 0.5f, MENU_Y - MENU_H * 0.5f + totalHeight * 0.5f, MENU_W, totalHeight, BG_COLOR.r, BG_COLOR.g, BG_COLOR.b, BG_COLOR.a);
        DrawMenuHeader("Car Shop", MENU_X, MENU_Y, MENU_W);

        float optionY = MENU_Y + MENU_H;
        for (size_t i = 0; i < g_vehicleCategories.size(); ++i) {
            DrawMenuOption(g_vehicleCategories[i].name, MENU_X, optionY + MENU_H * i, MENU_W, MENU_H, i == carCategoryIndex);
        }
        DrawMenuOption("Back", MENU_X, optionY + MENU_H * g_vehicleCategories.size(), MENU_W, MENU_H, g_vehicleCategories.size() == carCategoryIndex);

        // Navigation and activation logic
        if (IsKeyJustUp(VK_NUMPAD8) || IsKeyJustUp(VK_UP) || PadPressed(DPAD_UP)) carCategoryIndex = (carCategoryIndex - 1 + numOptions) % numOptions;
        if (IsKeyJustUp(VK_NUMPAD2) || IsKeyJustUp(VK_DOWN) || PadPressed(DPAD_DOWN)) carCategoryIndex = (carCategoryIndex + 1) % numOptions;
        if (IsKeyJustUp(VK_NUMPAD5) || IsKeyJustUp(VK_RETURN) || PadPressed(BTN_A)) {
            if (carCategoryIndex == g_vehicleCategories.size()) { // "Back" option selected
                menuCategory = 0; // CAT_MAIN
                menuIndex = 4;
                inputDelayFrames = 10; // Apply delay after action
            }
            else {
                inVehicleSelection = true;
                vehicleIndex = 0;
                inputDelayFrames = 10; // Apply delay after action
            }
        }
        // Added Back/Escape for going back from category selection
        if (IsKeyJustUp(VK_NUMPAD0) || IsKeyJustUp(VK_BACK) || IsKeyJustUp(VK_ESCAPE) || PadPressed(BTN_B)) {
            menuCategory = 0; // CAT_MAIN
            menuIndex = 4; // Index for Car Shop in main menu
            inputDelayFrames = 10; // Apply delay after action
        }
    }
    else {
        // --- Vehicle Selection ---
        VehicleCategory& selectedCategory = g_vehicleCategories[carCategoryIndex];
        const int numOptions = selectedCategory.vehicles.size() + 1;
        float totalHeight = MENU_H * (numOptions + 1);

        GRAPHICS::DRAW_RECT(MENU_X + MENU_W * 0.5f, MENU_Y - MENU_H * 0.5f + totalHeight * 0.5f, MENU_W, totalHeight, BG_COLOR.r, BG_COLOR.g, BG_COLOR.b, BG_COLOR.a);
        DrawMenuHeader(selectedCategory.name, MENU_X, MENU_Y, MENU_W);

        float optionY = MENU_Y + MENU_H;
        char vehicleLabel[100];
        for (size_t i = 0; i < selectedCategory.vehicles.size(); ++i) {
            sprintf_s(vehicleLabel, "$%d", selectedCategory.vehicles[i].price);
            DrawPairedMenuOption(selectedCategory.vehicles[i].name, vehicleLabel, MENU_X, optionY + MENU_H * i, MENU_W, MENU_H, i == vehicleIndex);
        }
        DrawMenuOption("Back", MENU_X, optionY + MENU_H * selectedCategory.vehicles.size(), MENU_W, MENU_H, selectedCategory.vehicles.size() == vehicleIndex);

        // Navigation and activation logic
        if (IsKeyJustUp(VK_NUMPAD8) || IsKeyJustUp(VK_UP) || PadPressed(DPAD_UP)) vehicleIndex = (vehicleIndex - 1 + numOptions) % numOptions;
        if (IsKeyJustUp(VK_NUMPAD2) || IsKeyJustUp(VK_DOWN) || PadPressed(DPAD_DOWN)) vehicleIndex = (vehicleIndex + 1) % numOptions;
        if (IsKeyJustUp(VK_NUMPAD5) || IsKeyJustUp(VK_RETURN) || PadPressed(BTN_A)) {
            if (vehicleIndex == selectedCategory.vehicles.size()) { // "Back" option selected
                inVehicleSelection = false;
                inputDelayFrames = 10; // Apply delay after action
            }
            else {
                VehicleForSale& car = selectedCategory.vehicles[vehicleIndex];
                if (Garage_HasVehicle(car.hash)) {
                    UI::_SET_NOTIFICATION_TEXT_ENTRY("STRING");
                    UI::_ADD_TEXT_COMPONENT_STRING("~y~You already own this vehicle!");
                    UI::_DRAW_NOTIFICATION(false, true);
                }
                else if (Money_Get() >= car.price) {
                    Money_Add(-car.price);
                    Garage_AddVehicle(car.hash);
                    UI::_SET_NOTIFICATION_TEXT_ENTRY("STRING");
                    UI::_ADD_TEXT_COMPONENT_STRING("~g~Purchase successful!");
                    UI::_DRAW_NOTIFICATION(false, true);
                }
                else {
                    UI::_SET_NOTIFICATION_TEXT_ENTRY("STRING");
                    UI::_ADD_TEXT_COMPONENT_STRING("~r~Not enough money!");
                    UI::_DRAW_NOTIFICATION(false, true);
                }
                inputDelayFrames = 10; // Apply delay after action
            }
        }
        // Added Back/Escape for going back from vehicle selection
        if (IsKeyJustUp(VK_NUMPAD0) || IsKeyJustUp(VK_BACK) || IsKeyJustUp(VK_ESCAPE) || PadPressed(BTN_B)) {
            inVehicleSelection = false;
            inputDelayFrames = 10; // Apply delay after action
        }
    }
}