#pragma once
#include "build.h"
#include <Arduino.h>

#define BIT_DELAY 1

struct Config
{
  // channels
  byte outputChannel[4];

  // MIDI
  byte velocity;
  bool pcEnabled;
  bool ccEnabled;
  bool realTimeEnabled;
  bool clockEnabled;

  // reader
  uint32_t byteDelay;
  bool experimentalCorrectionEnabled;
  
  // ADVANCE!
  bool ccMode[4];
  bool ccScaling[4]; //TODO: remove scaling feature limit to 0-112 or 0-15
  byte ccNumbers[4][7];
  byte groove;

  // eeprom version
  uint32_t version;
};

extern Config config;

void config_init();
void config_save();
void config_default();
