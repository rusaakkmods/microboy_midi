#pragma once
#include "config.h"

#define PPQN 24 // Pulses Per Quarter Note
#define MINIMUM_INTERVAL 13//8.3 //microseconds
#define MAXIMUM_INTERVAL 62.5 //microseconds

#define MINIMUM_BPM 40
#define MAXIMUM_BPM 300

typedef void (*TickCallback)();

struct Clock
{
    float interval;
    float correction;
    uint32_t bpm;

    TickCallback onTick;
};

extern Clock clock;

void clock_init();
void clock_reset();
void clock_tapTick();
void clock_handleOnTick(TickCallback callback);