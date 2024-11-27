#pragma once
#include "config.h"

#define PPQN 24 // Pulses Per Quarter Note
#define MINIMUM_INTERVAL 8333.3 //microseconds
#define MAXIMUM_INTERVAL 62500 //microseconds

typedef void (*TickCallback)();

struct Clock
{
    float interval;
    uint32_t bpm;

    TickCallback onTick;
};

extern Clock clock;

void clock_init();
void clock_reset();
void clock_tapTick();
void clock_handleOnTick(TickCallback callback);