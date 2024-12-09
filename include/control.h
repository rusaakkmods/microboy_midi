#pragma once
#include "config.h"

// rotary encoder for control and settings
#define ROTARY_CLK 9
#define ROTARY_DT 8
// rotary enter press
#define ROTARY_SW 7

// shift button
#define BUTTON_SHIFT 6

// mute toggle switch
#define MUTE_PU1 10
#define MUTE_PU2 16
#define MUTE_WAV 14
#define MUTE_NOI 15

void control_init();
void control_read();