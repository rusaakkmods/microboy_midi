#pragma once
#include "config.h"

struct Display
{
    uint32_t bpm;
    uint32_t velocity;
};

extern Display display;

void display_init();
void display_main();