#pragma once
#include "config.h"
#include "clock.h"

struct MIDIController
{
    volatile bool isSolo;
    volatile byte soloTrack;
    volatile bool isMute;

    volatile bool isPU1Muted;
    volatile bool isPU2Muted;
    volatile bool isWAVMuted;
    volatile bool isNOIMuted;
};

extern MIDIController midiController;

void midi_init();
void midi_flush();
void midi_handleStop();
void midi_realtime(byte command);
void midi_message(byte message, byte value);
void midi_experimentalCorrection(byte value);