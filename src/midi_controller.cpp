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

bool midi_isTrackMuted(byte track)
{
    switch (track)
    {
    case 0:
        return midiController.isPU1Muted;
    case 1:
        return midiController.isPU2Muted;
    case 2:
        return midiController.isWAVMuted;
    case 3:
        return midiController.isNOIMuted;
    default:
        return false;
    }
}

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

#ifndef USE_MIDI_h // NOT USE MIDI.h
    Serial1.write(message.byte1);
    Serial1.write(message.byte2);
    Serial1.write(message.byte3);
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
    byte vel = config.velocity;
    if ((midiController.isSolo && track != midiController.isSolo) || midiController.isMute) {
        vel = 0;
    }
    midi_send({0x09, static_cast<uint8_t>(0x90 | (config.outputChannel[track] - 1)), note, vel});

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

void midi_sendControlChange(byte track, byte value)
{
    byte ccNumber = 1; // Use the first CC number for Mode 0
    byte ccValue = value; 

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
        if (midi_isTrackMuted(track)) return;

        midi_sendNote(track, value);
    }
    else if (command < 0x08)
    { // 4-7 represent CC message
        track = command - 0x04;
        if (midi_isTrackMuted(track)) return;

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
        if (midi_isTrackMuted(track)) return;

        if (value == 0x7F) // use the GUNSHOT!!! y-FF command!
        {
            if (config.clockEnabled) midi_sendTick();
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
        if (midi_isTrackMuted(track)) return;
        // unknown! skip consume one value byte usually 0 or 127
    }
    // else
    // {
    //     return; // not supposed to happened!!
    // }
    lastTrack = track;
}

// Route Realtime MIDI Messages
// commands: 0x7D-Start, 0x7E-Stop, 0x7F-Idle
void midi_realtime(byte command)
{
    switch (command)
    {
    case 0x7D: // Start!
        if (config.realTimeEnabled && !startFlag)
        {
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

    Serial1.begin(31250); // DIN out

#ifdef USE_MIDI_h
    MIDI.begin(MIDI_CHANNEL_OMNI);
#endif

    midi_flush();
}