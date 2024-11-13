/*
enum MidiType: uint8_t
    InvalidType           = 0x00,    ///< For notifying errors
    NoteOff               = 0x80,    ///< Channel Message - Note Off
    NoteOn                = 0x90,    ///< Channel Message - Note On
    AfterTouchPoly        = 0xA0,    ///< Channel Message - Polyphonic AfterTouch
    ControlChange         = 0xB0,    ///< Channel Message - Control Change / Channel Mode
    ProgramChange         = 0xC0,    ///< Channel Message - Program Change
    AfterTouchChannel     = 0xD0,    ///< Channel Message - Channel (monophonic) AfterTouch
    PitchBend             = 0xE0,    ///< Channel Message - Pitch Bend
    SystemExclusive       = 0xF0,    ///< System Exclusive
	SystemExclusiveStart  = SystemExclusive,   ///< System Exclusive Start
    TimeCodeQuarterFrame  = 0xF1,    ///< System Common - MIDI Time Code Quarter Frame
    SongPosition          = 0xF2,    ///< System Common - Song Position Pointer
    SongSelect            = 0xF3,    ///< System Common - Song Select
    Undefined_F4          = 0xF4,
    Undefined_F5          = 0xF5,
    TuneRequest           = 0xF6,    ///< System Common - Tune Request
	SystemExclusiveEnd    = 0xF7,    ///< System Exclusive End
    Clock                 = 0xF8,    ///< System Real Time - Timing Clock
    Undefined_F9          = 0xF9,
    Tick                  = Undefined_F9, ///< System Real Time - Timing Tick (1 tick = 10 milliseconds)
    Start                 = 0xFA,    ///< System Real Time - Start
    Continue              = 0xFB,    ///< System Real Time - Continue
    Stop                  = 0xFC,    ///< System Real Time - Stop
    Undefined_FD          = 0xFD,
    ActiveSensing         = 0xFE,    ///< System Real Time - Active Sensing
    SystemReset           = 0xFF,    ///< System Real Time - System Reset

*/

#include <Arduino.h>
#include <EEPROM.h>
#include <MIDI.h>

#define PRODUCT_NAME "MEGABOY MIDI"
#define VERSION "v0.0.1"

MIDI_CREATE_INSTANCE(HardwareSerial, Serial1, MIDI);

#ifdef __AVR_ATmega2560__ // use board = megaatmega2560
#define USE_MEGA_2560_PRO
#elif __AVR_ATmega32U4__ // use board = leonardo
#define USE_PRO_MICRO
#endif

#if defined USE_MEGA_2560_PRO // TESTED on https://www.dreamgreenhouse.com/projects/arduino/mega2560pro.php

#define GB_CLOCK_PIN 22
#define GB_SO_PIN 23
#define GB_SI_PIN 24
#define MODE_KNOB_PIN A0

#elif defined USE_PRO_MICRO // TESTED on https://deskthority.net/wiki/Arduino_Pro_Micro

#define GB_CLOCK_PIN PIN_A0
#define GB_SO_PIN PIN_A1
#define GB_SI_PIN PIN_A2
#define MODE_KNOB_PIN A10

#endif

#define MODE_SLAVE 0
#define MODE_MASTER 1
#define MODE_MIDIOUT 2
#define MODE_MIDIMAP 3

unsigned int mode = MODE_SLAVE;
byte isClockMuted = false;

unsigned int channelPulse1 = 1;
unsigned int channelPulse2 = 2;
unsigned int channelWave = 3;
unsigned int channelNoise = 4;

unsigned int channelMaster = 16;

void sendClockPulse() {
  if (!isClockMuted) {
    /* Generate digital clock pulse
    / Generate digital clock pulse to gameboy
    / Basically sending 0101010101010101 each pulse
    / based on Arduinoboy https://github.com/trash80/Arduinoboy/blob/master/Arduinoboy/Mode_LSDJ_SlaveSync.ino
    / Thanks to @trash80
    */
    for(int ticks = 0; ticks < 8; ticks++) {
      digitalWrite(GB_CLOCK_PIN, LOW);
      digitalWrite(GB_CLOCK_PIN, HIGH);
    }
  }
}

void mutePulseClock() {
  isClockMuted = true;
}

void unmutePulseClock() {
  isClockMuted = false;
}

int modeReading() {
  int knobValue = analogRead(MODE_KNOB_PIN);
  const int snappedValues[] = {0, 1, 2, 3};
  int index = map(knobValue, 0, 1023, 0, 9);
  index = constrain(index, 0, 4);
  return snappedValues[index];
}

void setupSlaveMode() {
  pinMode(GB_CLOCK_PIN, OUTPUT); // set clock pin as output so we can send clock to gameboy

  MIDI.setHandleClock(sendClockPulse); // handle clock message
  MIDI.setHandleStart(unmutePulseClock); // handle start message
  MIDI.setHandleContinue(unmutePulseClock); // handle continue message
  MIDI.setHandleStop(unmutePulseClock); // handle stop message

  // Slave mode main loop
  while (modeReading() == MODE_SLAVE) {
    MIDI.read();
  }
}

const int pulseTimeOut = 16000;

void setupMasterMode() {
  pinMode(GB_CLOCK_PIN, INPUT);
  //MIDI.setThruFilterMode(midi::Thru::Off); // not sure if needed
  MIDI.disconnectCallbackFromType(midi::Clock);
  MIDI.disconnectCallbackFromType(midi::Start);
  MIDI.disconnectCallbackFromType(midi::Continue);
  MIDI.disconnectCallbackFromType(midi::Stop);

  byte isClockPulse = false;
  byte data = 0x00;
  byte fixedVelocity = 0x7F;
  int ticks = 0;
  do 
  {
    /* Reading Clock from gameboy 
    / This code is based on Arduinoboy
    / https://github.com/trash80/Arduinoboy/blob/master/Arduinoboy/Mode_LSDJ_MasterSync.ino
    / By @trash80
    */
    isClockPulse = digitalRead(GB_CLOCK_PIN);
    if (isClockPulse) { // if clock high is detected
      byte bit;
      int pulseTime = 0;
      while(isClockPulse) { // reading timeout check: based on Arduinoboy
        bit = digitalRead(GB_SI_PIN); // read Gameboy serial input

        pulseTime++;
        if(pulseTime > pulseTimeOut) { // when pulses too long considered as clock stop 
          isClockPulse = false;
          pulseTime = 0;
          
          MIDI.sendStop(); // send midi stop message
        }
        else {
          isClockPulse = digitalRead(GB_CLOCK_PIN); //read the clock
        }
      }
      pulseTime = 0;

      data = data << 1;        //left shift the serial byte by one to append new bit from last loop
      data = data + bit;       //and then add the bit that was read

      ticks++; 
      if(ticks <= 8) {
        MIDI.sendNoteOn(data, fixedVelocity, channelMaster); //TODO not sure why send note on
        MIDI.sendStart(); //TODO: is there a way not to send start along with clock
        MIDI.sendClock(); //sending clock here 
        ticks = 0;
        data = 0x00; //refresh data
      }
    }
  } while (modeReading() == MODE_MASTER);
}

void modeSwitch() {
  switch(modeReading()) {
    case MODE_SLAVE:
      setupSlaveMode();
      break;
    case MODE_MASTER:
      setupMasterMode();
      break;
    case MODE_MIDIOUT:
      //todo
      break;
    case MODE_MIDIMAP:
      //todo
      break;
    default:
      break;
  }
}

void setup() {
  pinMode(MODE_KNOB_PIN, INPUT);

  pinMode(GB_SI_PIN, INPUT);
  pinMode(GB_SO_PIN, OUTPUT);

  Serial1.begin(31250);
  MIDI.begin(MIDI_CHANNEL_OMNI);
}

void loop() {
  modeSwitch();
}