#include "reader.h"
#include "midi_controller.h"
#include <util/atomic.h>

uint64_t lastReadGameboy = 0;
bool isCommandWaiting = false;
byte commandWaiting = 0x00;
byte correctionByte = 0x00;

#ifdef ARDUINOBOY_READER
/* This one based on Arduinoboy version, it is more stable on higher tempo
// Arduinoboy (https://github.com/trash80/Arduinoboy)
// by Timothy Lamb @trash80
*/
byte reader_getByte()
{
    byte incomingMidiByte;
#ifndef NON_BLOCKING_DELAY
    delayMicroseconds(config.byteDelay);
#endif
    PORTF &= ~(1 << PF7); // LOW CLOCK_PIN
#ifndef NON_BLOCKING_DELAY
    delayMicroseconds(config.byteDelay);
#endif
    PORTF |= (1 << PF7); // HIGH CLOCK_PIN
    delayMicroseconds(BIT_DELAY);
    if (((PINF & (1 << PINF5)) ? 1 : 0))
    {
        incomingMidiByte = 0;
        for (int i = 0; i != 7; i++)
        {
            PORTF &= ~(1 << PF7); // LOW CLOCK_PIN
            delayMicroseconds(BIT_DELAY);
            PORTF |= (1 << PF7); // HIGH CLOCK_PIN
            incomingMidiByte = (incomingMidiByte << 1) + ((PINF & (1 << PINF5)) ? 1 : 0);
        }
        return incomingMidiByte;
    }
    return 0x7F;
}

#else

byte reader_getByte()
{
    byte receivedByte = 0;

    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) { // runs with interrupts disabled
        PORTF |= (1 << PF6); // making sure Gameboy Serial In is HIGH! explanation: docs/references/gb_link_serial_in.md
        for (uint8_t i = 0; i < 8; i++)
        {
            // PORTF &= ~(1 << PF7); // Set LOW to ensure
            // delayMicroseconds(BIT_DELAY);
            PORTF |= (1 << PF7);          // Set HIGH
            delayMicroseconds(BIT_DELAY); // Wait for the signal to stabilize

            receivedByte = (receivedByte << 1) + ((PINF & (1 << PINF5)) ? 1 : 0); // Read a bit, and shift it into the byte

            PORTF &= ~(1 << PF7); // Set LOW
            delayMicroseconds(BIT_DELAY);
        }
    }

#ifndef NON_BLOCKING_DELAY
    delayMicroseconds(config.byteDelay);
#endif

    return receivedByte &= 0x7F; // Set the MSB range value to 0-127
}
#endif

void reader_read()
{
#ifdef NON_BLOCKING_DELAY
    //TODO: this is problematics on higher tempo!! must adjust reduce dynamically based on tempo
    uint64_t currentTime = micros();
    if (currentTime - lastReadGameboy < config.byteDelay)
        return;
#endif

    byte value = reader_getByte();

    lastReadGameboy = micros();

    if (isCommandWaiting)
    {
        midi_message(commandWaiting, value);
        isCommandWaiting = false;
        commandWaiting = 0x00;
    }
    else if (value >= 0x7D)
    { // 125-127 realtime message
        midi_realtime(value);
    }
    else if (value >= 0x70)
    { // 112-124 command message
        isCommandWaiting = true;
        commandWaiting = value;
    }
    else
    { // 0 - 111 Hiccups!!! not supposed to happened!!
        if (config.experimentalCorrectionEnabled)
            midi_experimentalCorrection(value);
        // else ignore this!! beat skipp!
    }
}

void reader_init()
{
    pinMode(CLOCK_PIN, OUTPUT);
    pinMode(SI_PIN, INPUT_PULLUP);
    pinMode(SO_PIN, OUTPUT);

    // IMPORTANT! Gameboy Serial In must be HIGH explanation: docs/references/gb_link_serial_in.md
    digitalWrite(SO_PIN, HIGH);
}