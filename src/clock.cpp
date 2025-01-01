#include "clock.h"

Clock clock;

uint8_t ticks;
uint64_t lastTickTime;
const float CORRECTION_ALPHA = 0.2f;
#define GROOVE 6


void clock_init()
{
    clock_reset();
}

/**
 * @brief Resets the clock to its initial state.
 *
 * This function resets the clock by setting the tick count, last tick time,
 * interval, beats per minute (BPM), and correction factor to their initial values.
 */
void clock_reset()
{
    ticks = 0;
    lastTickTime = 0;
    clock.interval = 0.0;
    clock.bpm = 0.00;
    clock.correction = 0;
}

/**
 * @brief Calculates the clock tick and updates the interval and BPM.
 *
 * This function is responsible for calculating the clock tick based on the configured groove.
 * It updates the interval and BPM (beats per minute) based on the time elapsed since the last tick.
 * The interval and BPM are constrained within defined minimum and maximum values.
 * The function also handles the first tick separately to avoid incorrect calculations.
 *
 * @note The function increments the tick counter and resets it when it reaches PPQN (Pulses Per Quarter Note).
 *
 * @details
 * - If the current tick is a multiple of the configured groove, the function calculates the time interval
 *   since the last tick and updates the clock interval and BPM.
 * - The interval is calculated using a weighted average with a correction factor (CORRECTION_ALPHA).
 * - The interval is constrained between MINIMUM_INTERVAL and MAXIMUM_INTERVAL.
 * - The BPM is calculated from the interval and constrained between MINIMUM_BPM and MAXIMUM_BPM.
 * - The last tick time is updated to the current time plus a correction factor.
 * - The tick counter is incremented and reset to zero when it reaches PPQN.
 * 
 * @todo The function can be optimized further by reducing the number of calculations and constraints.
 */
void clock_calculateTick()
{
    if (ticks % GROOVE == 0)
    {
        uint64_t currentTime = micros();
        if (lastTickTime > 0) // skip on first tap
        {
            float interval = (currentTime - lastTickTime) / (GROOVE * 1000);
            interval = floor(CORRECTION_ALPHA * interval + (1 - CORRECTION_ALPHA) * clock.interval);
            interval = constrain(interval, MINIMUM_INTERVAL, MAXIMUM_INTERVAL);
            clock.interval = interval;

            float bpm = 60000.00 / (clock.interval * PPQN);
            //bpm = CORRECTION_ALPHA * bpm + (1 - CORRECTION_ALPHA) * clock.bpm;
            clock.bpm = constrain(bpm, MINIMUM_BPM, MAXIMUM_BPM);
        }
        lastTickTime = currentTime + (currentTime - clock.correction);
            
        // if (config.clockEnabled) clock_generateGroove(); // works well but dragged
    }
    ticks++;
    if (ticks == PPQN) ticks = 0;
}

/**
 * @brief Handles a tap tick event for the clock.
 *
 * This function is responsible for processing a tap tick event. If the clock
 * is enabled in the configuration, it triggers the clock's onTick() method.
 * Additionally, it calculates the tick using the clock_calculateTick() function.
 */
void clock_tapTick()
{
    if (config.clockEnabled) clock.onTick(); // this is sync but depends on reader

    clock_calculateTick();
}

/**
 * @brief Registers a callback function to be called on each clock tick.
 * 
 * This function sets the provided callback function to be executed 
 * whenever the clock generates a tick event.
 * 
 * @param callback The function to be called on each clock tick. 
 *                 It should match the TickCallback signature.
 */
void clock_handleOnTick(TickCallback callback)
{
    clock.onTick = callback;
}