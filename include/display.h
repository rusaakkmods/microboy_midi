#pragma once
#include "config.h"

#define NUM_MAIN_MENU 5

enum MenuState { 
    MAIN_DISPLAY, 
    MAIN_MENU, 
    SUBMENU 
};

enum ValueType {
    ON_OFF,
    RANGE_1_16,
    RANGE_0_127,
    RANGE_1000_5000_BY_100,
    ABOUT,
    SAVE_CONFIG,
    LOAD_DEFAULT
};

enum TriggerType {
    SOLO,
    MUTE
};

struct Cursor {
  uint8_t x;
  uint8_t y;
  uint8_t w;
  uint8_t h;
  ValueType type;
  TriggerType trigger;
};

struct SubMenu {
    const char* name PROGMEM;
    ValueType type;
    uint16_t* value;
    char* charValue;
};

struct MainMenu {
    const char* name PROGMEM;
    uint8_t size;
    const SubMenu* subMenus;
};

struct Display
{
    MenuState currentState;

    uint8_t mainCursorIndex;
    const Cursor* mainCursors;
    uint8_t cursorSize;

    uint8_t menuIndex;
    uint8_t submenuIndex;
    const MainMenu* mainMenus;
    uint8_t mainMenuSize;
};

extern Display display;

void display_init();
void display_refresh();
void display_disconnected();