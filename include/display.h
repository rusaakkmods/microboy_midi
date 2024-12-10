#pragma once
#include "config.h"

struct Display
{
    uint32_t bpm;
    byte velocity;
};

extern Display display;

void display_init();
void display_main();
void display_disconnected();