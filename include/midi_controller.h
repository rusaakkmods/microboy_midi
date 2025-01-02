#pragma once
#include "config.h"

struct MIDIController
{
    bool isSolo;
    byte soloTrack;
    bool isMute;

    bool isPU1Muted;
    bool isPU2Muted;
    bool isWAVMuted;
    bool isNOIMuted;
};

extern MIDIController midiController;

void midi_init();
void midi_flush();
void midi_handleStop();
void midi_realtime(byte command);
void midi_message(byte message, byte value);
void midi_experimentalCorrection(byte value);