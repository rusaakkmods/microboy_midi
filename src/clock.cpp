#include "clock.h"

Clock clock;

uint8_t ticks;
uint64_t lastTickTime;
float CORRECTION_ALPHA = 0.2f;

void clock_init()
{
    clock_reset();
}

void clock_reset()
{
    ticks = 0;
    lastTickTime = 0;
    clock.interval = 0.0;
    clock.bpm = 0.00;
    clock.correction = 0;
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

void clock_calculateTick()
{
    if (ticks % config.groove == 0)
    {
        uint64_t currentTime = micros();
        if (lastTickTime > 0) // skip on first tap
        {
            float interval = (currentTime - lastTickTime) / (config.groove * 1000);
            interval = floor(CORRECTION_ALPHA * interval + (1 - CORRECTION_ALPHA) * clock.interval);
            interval = constrain(interval, MINIMUM_INTERVAL, MAXIMUM_INTERVAL);
            clock.interval = interval;

            float bpm = 60000.00 / (clock.interval * PPQN);
            //bpm = CORRECTION_ALPHA * bpm + (1 - CORRECTION_ALPHA) * clock.bpm;
            clock.bpm = constrain(bpm, MINIMUM_BPM, MAXIMUM_BPM);
            // TODO: its near enough but need a little bit tweaking on byte offset correction
        }
        lastTickTime = currentTime + (currentTime - clock.correction);
            
        // if (config.clockEnabled) clock_generateGroove(); // works well but dragged
    }
    ticks++;
    if (ticks == PPQN) ticks = 0;
}

void clock_tapTick()
{
    if (config.clockEnabled) clock.onTick(); // this is sync but depends on reader

    clock_calculateTick();
}

void clock_handleOnTick(TickCallback callback)
{
    clock.onTick = callback;
}