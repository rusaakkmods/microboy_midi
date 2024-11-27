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
  bool ccScaling[4];
  byte ccNumbers[4][7];
  bool pcEnabled;
  bool ccEnabled;
  bool experimentalCorrectionEnabled;
  bool realTimeEnabled;
  bool clockEnabled;
  byte groove;
};

extern Config config;

void config_init();
void config_default();
