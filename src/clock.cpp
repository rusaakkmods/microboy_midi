#include "clock.h"

Clock clock;

uint8_t ticks;
uint64_t lastTickTime;

void clock_init()
{
    clock_reset();
}

void clock_reset()
{
    ticks = 0;
    lastTickTime = 0;
    clock.interval = 0.00;
    clock.bpm = 0;
}

void clock_generateGroove()
{
    if (!clock.onTick)
        return;

    for (uint8_t tick = 0; tick < config.groove; tick++)
    {
        clock.onTick();
        delayMicroseconds(clock.interval);
    }
}

void clock_tapTick()
{
    if (ticks++ % config.groove == 0)
    {
        uint64_t currentTime = micros();
        if (lastTickTime > 0) // skip on first tap
        {
            clock.interval = (currentTime - lastTickTime) / (config.groove * 1000);
            clock.bpm = round((60000.00 / (clock.interval * PPQN)) / 5) * 5;
            // TODO:
            // its near enough but need a little bit tweaking on byte offset correction
            // issue when multiple channels active at once
            // disrupted when there busy with midi activities
        }
        // TODO: make it dynamic by calculating tick delayed by midi activities
        // idea: need byte offset correction so its dynamic
        lastTickTime = currentTime - config.byteDelay;

        if (config.clockEnabled)
            clock_generateGroove();
    }

    if (ticks == PPQN)
        ticks = 0;
}

void clock_handleOnTick(TickCallback callback)
{
    clock.onTick = callback;
}