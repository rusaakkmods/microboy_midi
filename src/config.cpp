
#include "config.h"
#include <EEPROM.h>

#define EEPROM_CHECK_NUMBER 0x00000001 // random number to update this when EEPROM structure changed
#define EEPROM_CHECK_NUMBER_ADDRESS 0
#define EEPROM_CONFIG_ADDRESS sizeof(uint32_t)

Config config;

void config_default()
{
    // config channels
    config.outputChannel[0] = 1;
    config.outputChannel[1] = 2;
    config.outputChannel[2] = 3;
    config.outputChannel[3] = 4;

    // config MIDI
    config.velocity = 100;
    config.pcEnabled = false;
    config.ccEnabled = false;
    config.realTimeEnabled = true;
    config.clockEnabled = true;

    // config reader
    config.byteDelay = 2000; // at least 1000-5000 for stable reading
    config.experimentalCorrectionEnabled = true;

    // config ADVANCE!
    config.ccMode[0] = false;
    config.ccMode[1] = false;
    config.ccMode[2] = false;
    config.ccMode[3] = false;

    config.ccScaling[0] = false;
    config.ccScaling[1] = false;
    config.ccScaling[2] = false;
    config.ccScaling[3] = false;

    byte ccNumbersInit[4][7] = {
        {1, 2, 3, 7, 10, 11, 12},
        {1, 2, 3, 7, 10, 11, 12},
        {1, 2, 3, 7, 10, 11, 12},
        {1, 2, 3, 7, 10, 11, 12}};

    for (int i = 0; i < 4; i++)
    {
        for (int j = 0; j < 7; j++)
        {
            config.ccNumbers[i][j] = ccNumbersInit[i][j];
        }
    }
    config.groove = 6;

    // eeprom version
    config.version = EEPROM_CHECK_NUMBER;
}

void config_save()
{
    EEPROM.put(EEPROM_CHECK_NUMBER_ADDRESS, EEPROM_CHECK_NUMBER);
    EEPROM.put(EEPROM_CONFIG_ADDRESS, config);
}

void config_init()
{
    uint32_t magicNumber;
    EEPROM.get(EEPROM_CHECK_NUMBER_ADDRESS, magicNumber);

    if (magicNumber == EEPROM_CHECK_NUMBER)
    {
        EEPROM.get(EEPROM_CONFIG_ADDRESS, config);
    }
    else
    {
        config_default();
        config_save();
    }
}