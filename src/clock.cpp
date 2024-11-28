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
    clock.interval = 0.0;
    clock.bpm = 0;
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

float CORRECTION_ALPHA = 0.1f;
void clock_calculateTick()
{
    if (ticks++ % config.groove == 0)
    {
        uint64_t currentTime = micros();
        if (lastTickTime > 0) // skip on first tap
        {
            float interval = (currentTime - lastTickTime) / (config.groove * 1000);
            clock.interval = constrain(interval, MINIMUM_INTERVAL, MAXIMUM_INTERVAL);

            uint32_t bpm = round(60000.00 / (clock.interval * PPQN)); //round((60000.00 / (clock.interval * PPQN)) / 5) * 5;
            //bpm = CORRECTION_ALPHA * bpm + (1 - CORRECTION_ALPHA) * clock.bpm;
            clock.bpm = constrain(bpm, MINIMUM_BPM, MAXIMUM_BPM);
            // TODO:its near enough but need a little bit tweaking on byte offset correction
        }
        lastTickTime = currentTime + (currentTime - clock.correction);
            
        // if (config.clockEnabled) clock_generateGroove(); // works well but dragged
    }
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