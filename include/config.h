#pragma once
#include "build.h"
#include <Arduino.h>

#define BIT_DELAY 1

struct Config
{
  uint32_t version;

  uint32_t byteDelay;
  byte outputChannel[4];

  bool ccMode[4];
  bool ccScaling[4]; //TODO: remove scaling feature limit to 0-112 or 0-15
  byte ccNumbers[4][7];

  bool pcEnabled;
  bool ccEnabled;

  bool realTimeEnabled;
  bool clockEnabled;

  bool experimentalCorrectionEnabled;

  byte groove;
};

extern Config config;

void config_init();
void config_default();
