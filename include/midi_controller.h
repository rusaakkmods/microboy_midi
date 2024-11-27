#pragma once
#include "config.h"
#include "clock.h"

struct MIDIController
{
    volatile byte velocity;
};

extern MIDIController midiController;

void midi_init();
void midi_handleStop();
void midi_realtime(byte command);
void midi_message(byte message, byte value);
void midi_experimentalCorrection(byte value);