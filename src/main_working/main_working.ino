
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <MIDI.h> 

#define PRODUCT_NAME "MEGABOY MIDI"
#define VERSION "v0.0.1"

MIDI_CREATE_INSTANCE(HardwareSerial, Serial1, MIDI);

//MEGA 2560
#define GB_SET(bit_cl, bit_out, bit_in) PORTK = (PINK & B11111000) | ((bit_in<<2) | ((bit_out)<<1) | bit_cl)

#define GB_CLOCK_PIN 62//PIN_PK0
#define GB_SI_PIN 63//PIN_PK2 //NOTE: to Gameboy Serial out
#define GB_SO_PIN 64//PIN_PK1 //NOTE: to Gameboy Serial In

#define MODE_KNOB_PIN A0

// OLED display settings
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32
#define OLED_I2C_ADDRESS 0x3C

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

#define MODE_SLAVE 0
#define MODE_MASTER 1
#define MODE_MIDIOUT 2
#define MODE_MIDIMAP 3

unsigned int mode = MODE_SLAVE;


// unsigned int channelPulse1 = 1;
// unsigned int channelPulse2 = 2;
// unsigned int channelWave = 3;
// unsigned int channelNoise = 4;

void printDisplay() {
    display.clearDisplay();
    display.setCursor(0, 0);
    display.print("MEGABOY MIDI");
    display.setCursor(0, 10);
    display.print("MODE: ");
    String activeMode;
    switch (mode) {
        case 3: activeMode = "MI.MAP"; break;
        case 2: activeMode = "MI.OUT"; break;
        case 1: activeMode = "LSDJ (Master)"; break;
        case 0:
        default: activeMode = "MIDI (Slave)"; break;
    }
    display.print(activeMode);
    display.display();
}

byte isClockMuted = false;
// Generate digital clock pulse to gameboy
// Using macro function on PortA
// bassically sending byte 0,1,0,1,0,1,0,1 repeated 8 times
// based on Arduinoboy https://github.com/trash80/Arduinoboy/blob/master/Arduinoboy/Mode_LSDJ_SlaveSync.ino
// by @trash80
void sendClockPulse() {
  if (!isClockMuted) {
    for(int ticks = 0; ticks < 8; ticks++) {
      GB_SET(0, 0, 0);
      GB_SET(1, 0, 0);
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
  //detachInterrupt(GB_CLOCK_PIN);

  MIDI.setHandleClock(sendClockPulse); // handle clock message
  MIDI.setHandleStart(unmutePulseClock); // handle start message
  MIDI.setHandleContinue(unmutePulseClock); // handle continue message
  MIDI.setHandleStop(unmutePulseClock); // handle stop message

  // Slave mode main loop
  do {
    MIDI.read();
  } while (modeReading() == MODE_SLAVE);
}

unsigned int countPulse;
byte readClockLine;
bool sequencerStarted = false;
unsigned int ticks = 0;

// This code is based on Arduinoboy: https://github.com/trash80/Arduinoboy/blob/master/Arduinoboy/Mode_LSDJ_MasterSync.ino by @trash80
// Reading Clock Input from Gameboy detecting sequencer play and stop.
// I still can't figure out the clock timing, I set to 3 ticks per midi clock somehow sync!
// I hope someone explain this, it's definetely not 24 ppq the clock sending
// it's acurate enough but feels dragging!!!
void handleTicks(){
  if(!ticks) {
    if(!sequencerStarted) { 
      MIDI.sendRealTime(midi::Start);
      sequencerStarted = true;
    }
    MIDI.sendRealTime(midi::Clock);
    ticks = 0;
  }
  ticks++;
  if(ticks == 3) ticks = 0; // somehow 3 works instead of 8bit!! 
}

// This code is based on Arduinoboy: https://github.com/trash80/Arduinoboy/blob/master/Arduinoboy/Mode_LSDJ_MasterSync.ino by @trash80
// Reading Clock Input from Gameboy detecting sequencer play and stop.
// I'm bypassing the "note on" message for active LSDJ row/channel song position.
void masterReading() {
  readClockLine = digitalRead(GB_CLOCK_PIN); 
  if (readClockLine) {
    do { // read until false
      countPulse++;
      if (countPulse > 16000) { // to detect if lsdj sequencer stopped
        countPulse = 0;
        if (sequencerStarted) MIDI.sendRealTime(midi::Stop);
        sequencerStarted = false;
        break;
      }
    } while (digitalRead(GB_CLOCK_PIN)); //TODO: when sequencer Stop. its loop forever failed to switch mode, if exit when stop
    countPulse= 0;
    handleTicks(); // receiving ticks pulses
  }
}


void setupMasterMode() {
  pinMode(GB_CLOCK_PIN, INPUT);
  //attachInterrupt(GB_CLOCK_PIN, handleTicks, FALLING); //different pin on MEGA 2560, I'll do it later

  // MIDI.setThruFilterMode(midi::Thru::Off); // not sure if needed
  // MIDI.disconnectCallbackFromType(midi::Clock);
  // MIDI.disconnectCallbackFromType(midi::Start);
  // MIDI.disconnectCallbackFromType(midi::Continue);
  // MIDI.disconnectCallbackFromType(midi::Stop);
  MIDI.setHandleClock(mutePulseClock); // mute handle clock message
  MIDI.setHandleStart(mutePulseClock); // mute handle start message
  MIDI.setHandleContinue(mutePulseClock); // mute handle continue message
  MIDI.setHandleStop(mutePulseClock); // mute handle stop message

  do 
  {
    masterReading();
  } while (modeReading() == MODE_MASTER);
}

const int midioutBitDelay = 0;
byte incomingMidiByte;
boolean getIncommingSlaveByte()
{
  delayMicroseconds(midioutBitDelay);
  GB_SET(0, 0, 0);
  delayMicroseconds(midioutBitDelay);
  GB_SET(1, 0, 0);
  delayMicroseconds(2);
  if(digitalRead(GB_SI_PIN)) { //read
    incomingMidiByte = 0;
    for(int countPause = 0; countPause != 7; countPause++) {
      GB_SET(0, 0, 0);
      delayMicroseconds(2);
      GB_SET(1, 0, 0);
      incomingMidiByte = (incomingMidiByte << 1) + digitalRead(GB_SI_PIN); //read again
    }
    return true;
  }
  return false;
}

byte midiData[] = {0, 0, 0};
boolean midiValueMode = false;

// void stopNote(byte m)
// {
//   for(int x = 0; x<midioutNoteHoldCounter[m]; x++) {

//     midiEventPacket_t packet = { 0x08, 0x80 | memory[MEM_MIDIOUT_NOTE_CH+m], midioutNoteHold[m][x], 0 };
//     MidiUSB.sendMIDI(packet);
//     MidiUSB.flush();

//     MIDI.sendNoteOff(midioutNoteHold[m][x], 0, memory[MEM_MIDIOUT_NOTE_CH+m]+1);

//   }
//   midiOutLastNote[m] = -1;
//   midioutNoteHoldCounter[m] = 0;
// }

// void checkStopNote(byte m)
// {
//   if((midioutNoteTimer[m] + midioutNoteTimerThreshold) < millis()) {
//     stopNote(m);
//   }
// }

// void midioutDoAction(byte m, byte v)
// {
//   if(m < 4) {
//     note message
//     if(v) {
//       checkStopNote(m); //TODO: check if needed
//       playNote(m,v);
//     } else if (midiOutLastNote[m]>=0) {
//       stopNote(m);
//     }
//   } else if (m < 8) {
//     m-=4;
//     cc message
//     playCC(m,v);
//   } else if(m < 0x0C) {
//     m-=8;
//     playPC(m,v);
//   }
// }

// void stopAllNotes()
// {
//   stop all notes in each channel
//   for(int m = 0; m < 4; m++) {
//     if(midiOutLastNote[m] >= 0) {
//       stopNote(m);
//     }
//     midiData[0] = (0xB0 + (memory[MEM_MIDIOUT_NOTE_CH+m]));
//     midiData[1] = 123;
//     midiData[2] = 0x7F;
//     serial->write(midiData,3); //Send midi

// #ifdef USE_TEENSY
//     usbMIDI.sendControlChange(123, 127, memory[MEM_MIDIOUT_NOTE_CH+m]+1);
// #endif
// #ifdef USE_LEONARDO
//     midiEventPacket_t packet = {0x0B, 0xB0 | memory[MEM_MIDIOUT_NOTE_CH + m], 123, 127};
//     MidiUSB.sendMIDI(packet);
//     MidiUSB.flush();
// #endif
//   }
// }


void setupMIDIOutMode() {
  pinMode(GB_CLOCK_PIN, OUTPUT);
  digitalWrite(GB_CLOCK_PIN, HIGH);

  MIDI.setHandleClock(mutePulseClock); // mute handle clock message
  MIDI.setHandleStart(mutePulseClock); // mute handle start message
  MIDI.setHandleContinue(mutePulseClock); // mute handle continue message
  MIDI.setHandleStop(mutePulseClock); // mute handle stop message

  //countGbClockTicks=0;
  //lastMidiData[0] = -1;
  //lapstMidiData[1] = -1;
  //midiValueMode = false;

  do {
    if(getIncommingSlaveByte()) {
      if(incomingMidiByte > 0x6F) {
        switch(incomingMidiByte)
        {
          case 0x7F: //LSDJ header for clock tick
            MIDI.sendRealTime(midi::Clock);
            break;
          case 0x7E: //seq stop
            MIDI.sendRealTime(midi::Stop);
            stopAllNotes(); //not sure needed yet
            break;
          case 0x7D: //seq start
            MIDI.sendRealTime(midi::Start);
            break;
          default:
            midiData[0] = (incomingMidiByte - 0x70); //command
            midiValueMode = true;
            break;
        }
      } else if (midiValueMode == true) {
        midiValueMode = false;
        midioutDoAction(midiData[0], incomingMidiByte);
      }
    }
  } while (modeReading() == MODE_MIDIOUT);
}

/**
 * @brief Switches the operating mode of the device and sets up the corresponding mode.
 *
 * This function reads the current mode, updates the display, and sets up the device
 * according to the selected mode. It handles different modes such as slave, master,
 * MIDI out, and MIDI map. After setting up the mode, it sends a MIDI stop message.
 */
void modeSwitch() {
  mode = modeReading();
  printDisplay();
  switch(mode) {
    case MODE_SLAVE:
      setupSlaveMode();
      break;
    case MODE_MASTER:
      setupMasterMode();
      break;
    case MODE_MIDIOUT:
      setupMIDIOutMode();
      break;
    case MODE_MIDIMAP:
      //todo
      break;
    default:
      break;
  }
  MIDI.sendStop();
}

void setup() {
  pinMode(MODE_KNOB_PIN, INPUT);
  
  pinMode(GB_CLOCK_PIN, INPUT);
  pinMode(GB_SI_PIN, INPUT);
  pinMode(GB_SO_PIN, OUTPUT);

  digitalWrite(GB_CLOCK_PIN, HIGH);    // gameboy wants a HIGH line
  digitalWrite(GB_SO_PIN, LOW);    // no data to send

  Serial.begin(9600);
  Serial1.begin(31250);
  MIDI.begin(MIDI_CHANNEL_OMNI);

  // Initialize I2C for OLED
  Wire.begin();
  display.begin(SSD1306_SWITCHCAPVCC, OLED_I2C_ADDRESS);
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.clearDisplay();
  display.setCursor(0, 0);
  display.print(PRODUCT_NAME);
  display.print(" ");
  display.print(VERSION);
  display.setCursor(0, 10);
  display.print("Initializing..");
  display.setCursor(0, 20);
  display.print("rusaaKKMODS @ 2024");
  display.display();
}

void loop() {
  modeSwitch();
}