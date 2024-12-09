#include "reader.h"
#include "clock.h"
#include "midi_controller.h"

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
/**
 * @brief Reads a byte from the Gameboy serial input.
 *
 * This function reads a byte from the Gameboy serial input by toggling the
 * appropriate pins and waiting for the signal to stabilize. It ensures that
 * the Gameboy Serial In is HIGH before starting the read process. The function
 * reads each bit of the byte by checking the state of the PINF5 pin and shifts
 * it into the received byte. After reading all 8 bits, it optionally waits for
 * a configured byte delay before returning the received byte with the MSB set
 * to 0 (range 0-127).
 *
 * @return byte The byte read from the Gameboy serial input, with the MSB set to 0.
 */
byte reader_getByte()
{
    byte receivedByte = 0;
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

#ifndef NON_BLOCKING_DELAY
    delayMicroseconds(config.byteDelay);
#endif

    return receivedByte &= 0x7F; // Set the MSB range value to 0-127
}
#endif

/**
 * @brief Reads a byte from the reader and processes it as a MIDI message.
 * 
 * This function reads a byte from the reader and processes it based on its value.
 * It handles different types of MIDI messages, including real-time messages,
 * command messages, and experimental correction for unexpected values.
 * 
 * @note If NON_BLOCKING_DELAY is defined, the function will return early if the
 *       time since the last read is less than the configured byte delay.
 * 
 * @todo Adjust the delay dynamically based on the tempo to handle higher tempos.
 * 
 * The function performs the following steps:
 * 1. Checks if the delay since the last read is sufficient (if NON_BLOCKING_DELAY is defined).
 * 2. Reads a byte from the reader.
 * 3. Updates the last read time.
 * 4. Processes the byte based on its value:
 *    - If a command is waiting, it sends a MIDI message with the command and the byte.
 *    - If the byte is a real-time message (125-127), it processes it as a real-time MIDI message.
 *    - If the byte is a command message (112-124), it sets the command waiting flag.
 *    - If the byte is an unexpected value (0-111), it optionally performs experimental correction.
 * 5. Updates the correction time.
 */
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

    clock.correction = micros(); // set correction time
}

/**
 * @brief Initializes the reader by setting up the pin modes and initial states.
 *
 * This function configures the pin modes for the CLOCK_PIN, SI_PIN, and SO_PIN.
 * It sets the CLOCK_PIN and SO_PIN as outputs and the SI_PIN as an input.
 * Additionally, it sets the SO_PIN to HIGH, which is important for the Gameboy
 * Serial In functionality. For more details, refer to the documentation at
 * docs/references/gb_link_serial_in.md.
 */
void reader_init()
{
    pinMode(CLOCK_PIN, OUTPUT);
    pinMode(SI_PIN, INPUT_PULLUP);
    pinMode(SO_PIN, OUTPUT);

    // IMPORTANT! Gameboy Serial In must be HIGH explanation: docs/references/gb_link_serial_in.md
    digitalWrite(SO_PIN, HIGH);
}