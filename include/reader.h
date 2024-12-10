#pragma once
#include "config.h"

#define CLOCK_PIN A0 /* to be connected to Gameboy Clock */
#define SO_PIN A1    /* to be connected to Gameboy Serial In */
#define SI_PIN A2    /* to be connected to Gameboy Serial Out */

enum PinCheckState {
    CHECK_HIGH,
    CHECK_LOW,
    CHECK_RESTORE,
    CHECK_DONE
};

struct PinChecker {
    PinCheckState state;
    uint32_t stateStartTime;
    bool isConnected;
};

extern PinChecker pinChecker;

bool reader_checkConnection(void);
void reader_init();
void reader_read();