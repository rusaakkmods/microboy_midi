#include <Arduino.h>
#include "midi_controller.h"
#include <MIDIUSB.h>

MIDIController midiController;

#ifdef USE_MIDI_h
// MIDI Library for MIDI Serial Communication supposed to be faster but unstable
#include <MIDI.h>
MIDI_CREATE_INSTANCE(HardwareSerial, Serial1, MIDI);
#else
HardwareSerial &MIDI = Serial1;
#endif

// track 0-PU1, 1-PU2, 2-WAV, 3-NOI
byte lastNote[4];
bool stopFlag;
bool startFlag;
uint64_t idleTime;
byte lastTrack;

/**
 * @brief Flushes the MIDI buffer.
 *
 * This function flushes the MIDI buffer using the MidiUSB library. If the
 * USE_MIDI_h macro is defined, it reads from the MIDI interface. Otherwise,
 * it flushes the Serial1 buffer.
 */
void midi_flush()
{
    MidiUSB.flush();

#ifdef USE_MIDI_h
    MIDI.read();
#else
    Serial1.flush();
#endif
}

/**
 * @brief Sends a MIDI event packet.
 *
 * This function sends a MIDI event packet using the MidiUSB library. If the
 * USE_MIDI_h macro is not defined, it also sends the MIDI message bytes over
 * Serial1.
 *
 * @param message The MIDI event packet to send.
 */
void midi_send(midiEventPacket_t message)
{
    MidiUSB.sendMIDI(message);

#ifndef USE_MIDI_h // NOT USE MIDI.h
    Serial1.write(message.byte1);
    Serial1.write(message.byte2);
    Serial1.write(message.byte3);
#endif

    midi_flush();
}

/**
 * @brief Sends a MIDI clock tick message.
 *
 * This function sends a MIDI real-time clock message to synchronize MIDI devices.
 * If the `USE_MIDI_h` macro is defined, it uses the `MIDI.sendRealTime` method to send
 * the clock message. Additionally, it sends a custom MIDI message with the bytes
 * {0x0F, 0xF8, 0x00, 0x00}.
 */
void midi_sendTick()
{
#ifdef USE_MIDI_h
    MIDI.sendRealTime(midi::Clock);
#endif
    midi_send({0x0F, 0xF8, 0x00, 0x00});
}

/**
 * @brief Sends a MIDI Note Off message for the specified track.
 *
 * This function sends a MIDI Note Off message for the last note played on the given track.
 * It uses the MIDI library if available, and also sends a custom MIDI message.
 *
 * @param track The track number for which to send the Note Off message.
 */
void midi_noteOff(byte track)
{
    byte note = lastNote[track];
    if (note)
    {
#ifdef USE_MIDI_h
        MIDI.sendNoteOff(note, 0x00, config.outputChannel[track]);
#endif

        midi_send({0x08, static_cast<uint8_t>(0x80 | (config.outputChannel[track] - 1)), note, 0x00});

        lastNote[track] = 0;
    }
}

/**
 * @brief Sends a MIDI Note On message for a specified track and note.
 *
 * This function stops any previously playing note on the given track (assuming each track is monophonic),
 * and then sends a new Note On message for the specified note.
 *
 * @param track The track number to send the Note On message to.
 * @param note The MIDI note number to be played.
 */
void midi_noteOn(byte track, byte note)
{
    midi_noteOff(track); // stop previous note consider each track is monophonics

#ifdef USE_MIDI_h
    MIDI.sendNoteOn(note, midiController.velocity, config.outputChannel[track]);
#endif

    midi_send({0x09, static_cast<uint8_t>(0x90 | (config.outputChannel[track] - 1)), note, midiController.velocity});

    lastNote[track] = note;
}

/**
 * @brief Stops all MIDI notes on all tracks.
 *
 * This function iterates through all available tracks (0 to 3) and sends a
 * MIDI note off message to each track, effectively stopping any currently
 * playing notes.
 */
void midi_noteStop()
{
    for (uint8_t track = 0; track < 4; track++)
    {
        midi_noteOff(track);
    }
}

/**
 * @brief Sends a MIDI note on or off message based on the note value.
 *
 * This function sends a "Note On" message if the note value is greater than 0,
 * and a "Note Off" message if the note value is 0.
 *
 * @param track The MIDI track/channel to send the message on.
 * @param note The note value to send. A value between 1 and 127 sends a "Note On" message,
 *             while a value of 0 sends a "Note Off" message.
 */
void midi_sendNote(byte track, byte note)
{
    if (note)
    { // value > 0 then its a "Note On" 1-127 | LSDJ note range 0-119
        midi_noteOn(track, note);
    }
    else
    { // value 0 then its a "Note Off"
        midi_noteOff(track);
    }
}

/*  Reference from Arduinoboy (https://github.com/trash80/Arduinoboy)
    by Timothy Lamb @trash80
    ------------------------------------
    X## - Sends a MIDI CC -
    By default in Arduinoboy the high nibble selects a CC#, and the low nibble sends a value 0-F to 0-127.
    This can be changed to allow just 1 midi CC with a range of 00-6F, or 7 CCs with scaled or unscaled values.

    CC Mode:
    default: 1 for all 4 tracks
    - 0: use 1 midi CC, with the range of 00-6F,
    - 1: uses 7 midi CCs with the range of 0-F (the command's first digit would be the CC#)
    either way the value is scaled to 0-127 on output
    ------------------------------------
    CC Scaling:
    default: 1 for all 4 tracks
    - 1: scales the CC value range to 0-127 as oppose to lsdj's incomming 00-6F (0-112) or 0-F (0-15)
    - 0: no scaling, the value is directly sent to midi out 0-112 or 0-15
    scaling in c:
    byte scaledValue = map(value, 0, 112, 0, 127); // 0-112 to 0-127
    byte scaledValue = map(value, 0, 15, 0, 127); // 0-112 to 0-127
    ------------------------------------
    CC Message Numbers
    default options: {1,2,3,7,10,11,12} for each track, these options are CC numbers for lsdj midi out
    If CC Mode is 1, all 7 ccs options are used per channel at the cost of a limited resolution of 0-F
*/
void midi_sendControlChange(byte track, byte value)
{
    byte ccNumber;
    byte ccValue;

    if (config.ccMode[track])
    {
        if (config.ccScaling[track])
        {
            // CC Mode 1 with scaling
            ccNumber = config.ccNumbers[track][(value & 0xF0) >> 4]; // High nibble
            ccValue = map(value & 0x0F, 0, 15, 0, 127);              // Low nibble
        }
        else
        {
            // CC Mode 1 without scaling
            ccNumber = config.ccNumbers[track][(value & 0xF0) >> 4]; // High nibble
            ccValue = value & 0x0F;                                  // Low nibble
        }
    }
    else
    {
        if (config.ccScaling[track])
        {
            // CC Mode 0 with scaling
            ccNumber = config.ccNumbers[track][0]; // Use the first CC number for Mode 0
            ccValue = map(value, 0, 112, 0, 127);  // Scale the value
        }
        else
        {
            // CC Mode 0 without scaling
            ccNumber = config.ccNumbers[track][0]; // Use the first CC number for Mode 0
            ccValue = value;                       // Direct value
        }
    }

#ifdef USE_MIDI_h
    MIDI.sendControlChange(ccNumber, ccValue, config.outputChannel[track]);
#endif

    midi_send({0x0B, static_cast<uint8_t>(0xB0 | (config.outputChannel[track] - 1)), ccNumber, ccValue});

#ifdef DEBUG_MODE
    Serial.print("CC Command - track:");
    Serial.print(track);
    Serial.print(" value:");
    Serial.println(value);
#endif
}

/**
 * @brief Sends a MIDI Program Change message to the specified track.
 *
 * This function sends a MIDI Program Change message using the specified track and value.
 * If the USE_MIDI_h macro is defined, it uses the MIDI library to send the message.
 * Otherwise, it sends the message using the midi_send function.
 * If DEBUG_MODE is defined, it also prints debug information to the Serial monitor.
 *
 * @param track The track number to which the Program Change message will be sent.
 * @param value The program number to be sent in the Program Change message.
 */
void midi_sendProgramChange(byte track, byte value)
{
#ifdef USE_MIDI_h
    MIDI.sendProgramChange(value, config.outputChannel[track]);
#endif

    midi_send({0x0C, static_cast<uint8_t>(0xC0 | (config.outputChannel[track] - 1)), value, 0x00});

#ifdef DEBUG_MODE
    Serial.print("PC Command - track:");
    Serial.print(track);
    Serial.print(" value:");
    Serial.println(value);
#endif
}

/**
 * @brief Experimental function to correct Gameboy reading hiccups!!.
 *
 * This function attempts to correct Gameboy reading hiccups by sending a note
 * using the last track if the provided value is not zero. If the value
 * is zero, it is ignored as it indicates an idle state.
 *
 * @param value The MIDI value to be corrected.
 *
 * @note This function is experimental
 */
void midi_experimentalCorrection(byte value)
{
    // EXPERIMENTAL HICCUPS! CORRECTION!! LOL
    if (value > 0)
    {
#ifdef DEBUG_MODE
        Serial.print("Hiccups!!: ");
        Serial.println(value);
#endif
        midi_sendNote(lastTrack, value);
    }
}

/**
 * @brief Handles the MIDI stop event.
 *
 * This function is responsible for managing the MIDI stop event, ensuring that
 * the "stop" glitch is avoided by checking the stop flag and idle time. If the
 * conditions are met, it sends a MIDI stop message, resets the clock, stops
 * any active MIDI notes, and flushes the MIDI buffer.
 *
 * @note Conditions:
 * @note - The stop flag must be set.
 * @note - The idle time must be greater than 100.
 *
 * @details Actions:
 * - If real-time MIDI is enabled, sends a MIDI stop message.
 * - Resets the clock.
 * - Stops any active MIDI notes.
 * - Flushes the MIDI buffer.
 */
void midi_handleStop()
{
    if (stopFlag && idleTime > 100) // at least 300 idle time to consider stop
    {
        if (config.realTimeEnabled)
        {
#ifdef USE_MIDI_h
            MIDI.sendRealTime(midi::Stop);
#endif

            midi_send({0x0F, 0xFC, 0x00, 0x00});
        }

        clock_reset();
        midi_noteStop();
        midi_flush();

#ifdef DEBUG_MODE
        Serial.println("Stop!");
#endif

        startFlag = false;
        stopFlag = false;
        idleTime = 0;
    }
}

/**
 * @brief Handles incoming MIDI messages and processes them based on the command type.
 *
 * This function processes different types of MIDI messages such as Note On/Off, Control Change (CC),
 * and Program Change (PC). It also handles experimental correction and clock tap tick based on the
 * configuration settings.
 *
 * @param message The MIDI message byte, which includes the command type and channel information.
 * @param value The value byte associated with the MIDI message, typically representing velocity or control value.
 *
 * @details Command types:
 * - 0x00 to 0x03: Note On/Off messages for tracks 0 to 3.
 * - 0x04 to 0x07: Control Change (CC) messages for tracks 0 to 3.
 * - 0x08 to 0x0B: Program Change (PC) messages for tracks 0 to 3.
 * - 0x0C to 0x0F: Reserved for future use or unknown commands.
 */
void midi_message(byte message, byte value)
{
    idleTime = 0;
    stopFlag = false;

    byte command = message - 0x70;
    byte track = 0;
    if (command < 0x04)
    { // 0-3 represent Track index
        track = command;
        midi_sendNote(track, value);
    }
    else if (command < 0x08)
    { // 4-7 represent CC message
        track = command - 0x04;
        if (config.ccEnabled)
        {
            midi_sendControlChange(track, value);
        }
        else
        {
            if (config.experimentalCorrectionEnabled)
                midi_experimentalCorrection(value);
        }
#ifdef DEBUG_MODE
        Serial.println("Control Change!");
#endif
    }
    else if (command < 0x0C)
    { // 8-11 represent PC message
        track = command - 0x08;
        if (value == 0x7F) // use the GUNSHOT!!! y-FF command!
        {
            clock_tapTick();
        }
        else
        {
            if (config.pcEnabled)
            {
                midi_sendProgramChange(track, value);
            }
            else
            {
                if (config.experimentalCorrectionEnabled)
                    midi_experimentalCorrection(value);
            }
#ifdef DEBUG_MODE
            Serial.println("Program Change!");
#endif
        }
    }
    else if (command <= 0x0F)
    { // 12-15 not yet sure!
        track = command - 0x0C;
        // unknown! skip consume one value byte usually 0 or 127
    }
    else
    {
        return; // not supposed to happened!!
    }
    lastTrack = track;
}

// Route Realtime MIDI Messages
// commands: 0x7D-Start, 0x7E-Stop, 0x7F-Idle
/**
 * @brief Handles MIDI real-time messages.
 *
 * This function processes incoming MIDI real-time messages and performs actions
 * based on the command received. It supports Start (0x7D), Stop (0x7E), and Idle (0x7F)
 * commands. The function also includes debug output for troubleshooting.
 *
 * @param command The MIDI real-time command byte.
 *
 * @details Command values:
 * - 0x7D: Start - Resets the clock, sends a start message, and sets the start flag.
 * - 0x7E: Stop - Sets the stop flag to debounce stop messages.
 * - 0x7F: Idle - Increments idle time if the stop flag is set.
 */
void midi_realtime(byte command)
{
    switch (command)
    {
    case 0x7D: // Start!
        if (config.realTimeEnabled && !startFlag)
        {
            clock_reset();

            midi_send({0x0F, 0xFA, 0x00, 0x00});

#ifdef USE_MIDI_h
            MIDI.sendRealTime(midi::Start);
#endif

            startFlag = true;
            stopFlag = false;

#ifdef DEBUG_MODE
            Serial.println("Start");
#endif
        }
#ifdef DEBUG_MODE
        else if (startFlag)
        {
            Serial.println("Start Hiccups!");
        }
#endif
        break;
    case 0x7E: // Stop!!
        if (config.realTimeEnabled && !stopFlag)
            stopFlag = true; // debounce stop in case of hiccups
        break;
    case 0x7F: // consider this idle
        if (stopFlag)
            idleTime++;
        // microboy byte reading clock!! ignore for now.. very missleading....
        break;
    default:
#ifdef DEBUG_MODE
        Serial.print("Unknown Realtime: ");
        Serial.print(command);
#endif
        break;
    }
}

/**
 * @brief Initializes the MIDI controller.
 *
 * This function sets up the initial state for the MIDI controller, including
 * resetting note states, track states, and flags. It also initializes the
 * MIDI communication and clock handling.
 *
 * @details The following initializations are performed:
 * - Resets the last note states for PU1, PU2, WAV, and NOI channels.
 * - Resets the last track state.
 * - Resets the stop and start flags.
 * - Resets the idle time.
 * - Sets the default velocity for the MIDI controller.
 * - Initializes the Serial1 communication at 31250 baud rate for DIN out.
 * - Initializes the MIDI library if it is being used.
 * - Flushes any pending MIDI messages.
 * - Initializes the clock and sets up the tick handler for sending MIDI ticks.
 */
void midi_init()
{
    lastNote[0] = 0; // PU1
    lastNote[1] = 0; // PU2
    lastNote[2] = 0; // WAV
    lastNote[3] = 0; // NOI

    lastTrack = 0;
    stopFlag = false;
    startFlag = false;
    idleTime = 0;

    midiController.velocity = 100;

    Serial1.begin(31250); // DIN out

#ifdef USE_MIDI_h
    MIDI.begin(MIDI_CHANNEL_OMNI);
#endif

    midi_flush();

    clock_init();
    clock_handleOnTick(midi_sendTick);
}