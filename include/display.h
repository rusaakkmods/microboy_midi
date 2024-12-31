#pragma once
#include "config.h"

enum ValueType {
    ON_OFF,
    VALUE_1_16,
    VALUE_0_127
};

struct ItemValue
{
    ValueType type;
    byte min;
    byte max;
    byte value;
};

struct SubMenu
{
    char *name;
    ItemValue value;
};

struct MenuItem
{
    char *name;
    SubMenu *subMenu[];
};

struct Display
{
    uint32_t bpm;
    byte velocity;
    byte mute_pu1;
    byte mute_pu2;
    byte mute_wav;
    byte mute_noi;
};

extern MenuItem mainMenu[];

extern Display display;

void display_init();
void display_main();
void display_disconnected();