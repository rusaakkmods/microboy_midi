#pragma once
#include "config.h"

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
    TEXT,
    ACTION
};

enum TriggerType {
    SOLO,
    MUTE
};

struct Cursor {
  int x;
  int y;
  int w;
  int h;
  int value;
  ValueType type;
  TriggerType trigger;
};

struct SubMenu {
    char* name;
    ValueType type;
    char* value;
};

struct MainMenu {
    char* name;
    uint8_t size;
    SubMenu* subMenus;
};

struct Display
{
    MenuState currentState;

    int mainCursorIndex;
    Cursor* mainCursors;
    int cursorSize;

    int menuIndex;
    int submenuIndex;
    MainMenu* mainMenus;
    int mainMenuSize;
};

extern Display display;

void display_init();
void display_refresh();
void display_disconnected();