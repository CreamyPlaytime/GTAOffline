#pragma once

#include "script.h"

// Initializes the credits screen (currently does nothing)
void Credits_Init();

// Draws the credits menu (now interactive)
void draw_credits_menu();

// Optional: expose open_link() if you want to call it elsewhere
void open_link(const char* url);

// Optional: expose the CreditEntry struct if you want to reuse the layout elsewhere
struct CreditEntry {
    const char* label;
    const char* url; // nullptr if not clickable
};
