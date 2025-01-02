#pragma once
#include "build.h"
#include <Arduino.h>

#define BIT_DELAY 1

//NOTE: configurabe only use uint16_t
struct Config
{
  // channels
  uint16_t outputChannel[4];

  // MIDI
  uint16_t velocity;
  uint16_t pcEnabled;
  uint16_t ccEnabled;
  uint16_t realTimeEnabled;
  uint16_t clockEnabled;

  // reader
  uint16_t byteDelay;
  uint16_t experimentalCorrectionEnabled;

  // eeprom version
  uint32_t version;
};

extern Config config;

void config_init();
void config_save();
void config_default();
