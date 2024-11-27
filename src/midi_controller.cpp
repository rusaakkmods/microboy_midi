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
byte lastTrack; // Track with the shortest update interval

void midi_flush()
{
    MidiUSB.flush();

#ifdef USE_MIDI_h
    MIDI.read();
#else
    Serial1.flush();
#endif
}

void midi_send(midiEventPacket_t message)
{
    MidiUSB.sendMIDI(message);

#ifndef USE_MIDI_h //NOT USE MIDI.h
    if (Serial1.available()) { 
        Serial1.write(message.byte1);
        Serial1.write(message.byte2);
        Serial1.write(message.byte3);
    }
#endif

    midi_flush();
}

void midi_sendTick()
{
#ifdef USE_MIDI_h
    MIDI.sendRealTime(midi::Clock);
#endif
    midi_send({0x0F, 0xF8, 0x00, 0x00});
}

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

void midi_noteOn(byte track, byte note)
{
    midi_noteOff(track); // stop previous note consider each track is monophonics

#ifdef USE_MIDI_h
    MIDI.sendNoteOn(note, midiController.velocity, config.outputChannel[track]);
#endif

    midi_send({0x09, static_cast<uint8_t>(0x90 | (config.outputChannel[track] - 1)), note, midiController.velocity});

    lastNote[track] = note;
}

void midi_noteStop()
{
    for (uint8_t track = 0; track < 4; track++)
    {
        midi_noteOff(track);
    }
}

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

/* Reference from Arduinoboy (https://github.com/trash80/Arduinoboy)
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

// experimental hiccups correction
void midi_experimentalCorrection(byte value)
{
#ifdef DEBUG_MODE
    Serial.print("Hiccups!!: ");
    Serial.println(value);
#endif
    // EXPERIMENTAL HICCUPS! CORRECTION!! LOL
    if (value == 0)
    {
        // ignore this!!! when its idle always 0
    }
    else
    { // use the last track
        midi_sendNote(lastTrack, value);
    }
}

void midi_handleStop()
{                                                                 // avoiding the "stop" glitch
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
    

#ifdef DEBUG_MODE
    Serial.begin(9600); // to serial monitor
#else
    Serial.begin(115200); // to to USB
#endif
    // Initialize MIDI Serial Communication
    Serial1.begin(31250);

#ifdef USE_MIDI_h
    MIDI.begin(MIDI_CHANNEL_OMNI);
#endif

    midi_flush();

    clock_init();
    clock_handleOnTick(midi_sendTick);
}