#pragma once
#include "config.h"

// rotary encoder for control and settings
#define ROTARY_CLK 9
#define ROTARY_DT 8
#define ROTARY_SW 7

void control_init();
void control_read();