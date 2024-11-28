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
    clock.interval = MINIMUM_INTERVAL;
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

float ALPHA = 0.1;
void clock_tapTick()
{
    if (config.clockEnabled) clock.onTick();

    if (ticks++ % config.groove == 0)
    {
        uint64_t currentTime = micros();
        if (lastTickTime > 0) // skip on first tap
        {
            float interval = (currentTime - lastTickTime) / (config.groove * 1000);
            clock.interval = ALPHA * interval + (1 - ALPHA) * clock.interval;
            clock.interval = constrain(clock.interval, MINIMUM_INTERVAL, MAXIMUM_INTERVAL);

            uint32_t bpm = round(60000.00 / (clock.interval * PPQN)); //round((60000.00 / (clock.interval * PPQN)) / 5) * 5;
            clock.bpm = ALPHA * bpm + (1 - ALPHA) * clock.bpm;
            clock.bpm = constrain(clock.bpm, MINIMUM_BPM, MAXIMUM_BPM);
            // TODO:
            // its near enough but need a little bit tweaking on byte offset correction
            // issue when multiple channels active at once
            // disrupted when there busy with midi activities
        }
        // TODO: make it dynamic by calculating tick delayed by midi activities
        // idea: need byte offset correction so its dynamic
        lastTickTime = currentTime - config.byteDelay;
            
        //clock_generateGroove(); //crash the buffer
    }

    if (ticks == PPQN)
        ticks = 0;
}

void clock_handleOnTick(TickCallback callback)
{
    clock.onTick = callback;
}