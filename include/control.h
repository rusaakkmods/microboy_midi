#pragma once
#include "config.h"

// Pin definitions
#define ENCODER_PIN_A 8  // PCINT5
#define ENCODER_PIN_B 9  // PCINT4
#define BUTTON_PIN 7
#define SHIFT_PIN 6

// mute toggle switch
#define MUTE_PU1 10
#define MUTE_PU2 16
#define MUTE_WAV 14
#define MUTE_NOI 15

void control_init();
void control_read();