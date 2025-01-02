#pragma once
#include "config.h"

#define CLOCK_PIN A0 /* to be connected to Gameboy Clock */
#define SO_PIN A1    /* to be connected to Gameboy Serial In */
#define SI_PIN A2    /* to be connected to Gameboy Serial Out */

void reader_init();
void reader_read();