
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