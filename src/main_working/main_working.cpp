
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <MIDI.h> 

#define PRODUCT_NAME "MEGABOY MIDI"
#define VERSION "v0.0.1"

//i'll use Pro Micro later for release version
MIDI_CREATE_INSTANCE(HardwareSerial, Serial1, MIDI);

// This macro function is based on Arduinoboy: https://github.com/trash80/Arduinoboy/blob/master/Arduinoboy/Arduinoboy.ino
// by @trash80
// it is proven to work faster than digitalWrite which can only handle 1 pin at a time, this macro can handle 3 pins at once
// choose any availablee port on your board, in this case I'm using PORTK PIN 0,1,2
#define GB_SET(bit_cl, bit_out, bit_in) PORTK = (PINK & B11111000) | ((bit_in<<2) | ((bit_out)<<1) | bit_cl)
#define GB_CLOCK_PIN 62 //PIN_PK0 //NOTE: to Gameboy Clock
#define GB_SI_PIN 63 //PIN_PK1 //NOTE: to Gameboy Serial out
#define GB_SO_PIN 64 //PIN_PK2 //NOTE: to Gameboy Serial In

// LSDJ default MIDI Out Channel Mapping
// Carefull with this, make sure your LSDJ MIDI Out Channel Mapping is set as default
// otherwise you need to adjust this mapping
#define CHANNEL_PU1 1
#define CHANNEL_PU2 2
#define CHANNEL_WAV 3
#define CHANNEL_NOI 4

#pragma region Display Setup
// OLED display
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32
#define OLED_I2C_ADDRESS 0x3C

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

uint8_t mode;
uint16_t bpm;
unsigned long lastBPMTime = 0;
unsigned long clockCount = 0;

void printDisplay() {
    //todo: this is laggish, I'll fix it later and maybe update to larger oled display 128x64 using u8g2 faster refresh rate
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
    display.setCursor(0, 20);
    display.print("BPM: ");
    display.print(bpm);
    display.display();
}

// Update BPM every second
void calculateBPM() {
  clockCount++;
  unsigned long currentTime = millis();
  if (currentTime - lastBPMTime >= 1000) { // Calculate BPM every second
      bpm = (clockCount * 60 * 4) / (24 * 4);
      lastBPMTime = currentTime;
      // Serial.print("[");
      // Serial.print(bpm);
      // Serial.println("]");
      printDisplay();
      clockCount = 0; // Reset the clock count after calculation
  }
}

#pragma endregion

#pragma region Sync Mode
// LSDJ Sync Mode
#define MODE_KNOB_PIN A0
#define MODE_SLAVE 0
#define MODE_MASTER 1
#define MODE_MIDIOUT 2
#define MODE_MIDIMAP 3


int modeReading() {
  int knobValue = analogRead(MODE_KNOB_PIN);
  const int snappedValues[] = {MODE_SLAVE, MODE_MASTER, MODE_MIDIOUT, MODE_MIDIMAP};
  int index = map(knobValue, 0, 1023, 0, 4);
  index = constrain(index, 0, 4);
  return snappedValues[index];
}

#pragma endregion

#pragma region MIDI Clock Callback Handler
// MIDI Clock Callback Handler
boolean isClockMuted = false;
void mutePulseClock() {
  isClockMuted = true;
}

void unmutePulseClock() {
  isClockMuted = false;
}
#pragma endregion

#pragma region LSDJ Slave Mode
// based on Arduinoboy https://github.com/trash80/Arduinoboy/blob/master/Arduinoboy/Mode_LSDJ_SlaveSync.ino
// by @trash80
// Generate and send digital clock pulse to Gameboy
// Bassically sending byte 0,1,0,1,0,1,0,1 repeatedly 8 times
void sendClockPulse() {
  if (!isClockMuted) {
    for(int ticks = 0; ticks < 8; ticks++) {
      //PORTK = B00000000;
      //PORTK = B00000001;
      GB_SET(0, 0, 0);
      GB_SET(1, 0, 0);
    }
  }
}

void setupSlaveMode() {
  pinMode(GB_CLOCK_PIN, OUTPUT);
  //detachInterrupt(GB_CLOCK_PIN); //if interrupt is used, I'll do it later

  MIDI.setHandleClock(sendClockPulse); // handle clock message
  MIDI.setHandleStart(unmutePulseClock); // handle start message
  MIDI.setHandleContinue(unmutePulseClock); // handle continue message
  MIDI.setHandleStop(unmutePulseClock); // handle stop message

  // Slave mode main loop
  do {
    MIDI.read();
  } while (modeReading() == MODE_SLAVE);
}

#pragma endregion

#pragma region LSDJ Master Mode

boolean isSequencing = false;
byte ticks = 0;

// This code is based on Arduinoboy: https://github.com/trash80/Arduinoboy/blob/master/Arduinoboy/Mode_LSDJ_MasterSync.ino 
// by @trash80
// Reading Clock Input from Gameboy detecting sequencer play and stop.
// I still can't figure out the clock timing, I set to 3 ticks per midi clock somehow sync!
// I hope someone explain this, it's definetely not 24 ppq the clock sending
// it's acurate enough but feels dragging!!! perhaps my code is wrong or mega works in different cycle! but it works
void handleTicks(){
  if(!ticks) {
    if(!isSequencing) { 
      MIDI.sendRealTime(midi::Start);
      isSequencing = true;
    }
    MIDI.sendRealTime(midi::Clock);
    calculateBPM();
    ticks = 0;
  }
  ticks++;
  if(ticks == 3) ticks = 0; // somehow 4 ticks works instead of 8!! 
}

// This code is based on Arduinoboy: https://github.com/trash80/Arduinoboy/blob/master/Arduinoboy/Mode_LSDJ_MasterSync.ino
// by @trash80
// if stays HIGH for at least 16000 cycles, it indicates sequencer stopped
boolean checkStopped(uint16_t countHigh) {
  if(countHigh > 16000) {
    MIDI.sendRealTime(midi::Stop);
    isSequencing = false;
    return true;
  }
  return false;
}

// This code is based on Arduinoboy: https://github.com/trash80/Arduinoboy/blob/master/Arduinoboy/Mode_LSDJ_MasterSync.ino
// by @trash80
// Reading Clock Input from Gameboy detecting sequencer stop.
// When reading LOW in Clock Input, it indicates sequencer playing and sending ticks
// I'm bypassing the "note on" message for active LSDJ row/channel song position.
void masterReading() {
  uint16_t countHigh = 0;
  if (digitalRead(GB_CLOCK_PIN)) { // initial high skipping any LOW after
    while (digitalRead(GB_CLOCK_PIN)) { // read again until clock line is LOW or Stopped
      if (checkStopped(countHigh++)) break;
    }
    handleTicks();
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

#pragma endregion

const int midioutBitDelay = 0;
byte incomingMidiByte;
// it is writing to clock pin not yet sure why sending 0, 1, 0, 1, 0, 1, 0, 1 pulse similar to slave mode
boolean getIncommingSlaveByte()
{
  delayMicroseconds(midioutBitDelay);
  GB_SET(0, 0, 0);
  //PORTK = B00000000;
  delayMicroseconds(midioutBitDelay);
  GB_SET(1, 0, 0);
  //PORTK = B00000001;
  delayMicroseconds(2);
  if(digitalRead(GB_SI_PIN)) { //read
  //if(PINK & B00000100) {
    incomingMidiByte = 0;
    for(int countPause = 0; countPause != 7; countPause++) {
      GB_SET(0, 0, 0);
      //PORTK = B00000000;
      delayMicroseconds(2);
      GB_SET(1, 0, 0);
      //PORTK = B00000001;
      incomingMidiByte = (incomingMidiByte << 1) + digitalRead(GB_SI_PIN); //read again
      //incomingMidiByte = (incomingMidiByte << 1) + ((PINK & B00000100)>>2);
    }
    return true;
  }
  return false;
}

byte midiData[] = {0, 0, 0};
boolean midiValueMode = false;
int midiOutLastNote[4] = {-1,-1,-1,-1};
byte midioutNoteHoldCounter[4];
byte midioutNoteHold[4][4];
unsigned long midioutNoteTimer[4];
int midioutNoteTimerThreshold = 10;

void checkStopNote(byte channel) //check if timer is expired
{
  if((midioutNoteTimer[channel] + midioutNoteTimerThreshold) < millis()) {
    stopNote(channel);
  }
}

void playNote(byte channel, byte value)
{
  MIDI.sendNoteOn(value, 0x7F, channel+1);

  midioutNoteHold[channel][midioutNoteHoldCounter[channel]] = value;
  midioutNoteHoldCounter[channel]++;
  midioutNoteTimer[channel] = millis();
  midiOutLastNote[channel] = value;
}

void playCC(byte channel, byte value)
{
  // byte v = value;

  // if(memory[MEM_MIDIOUT_CC_MODE+channel]) { //CC MODE related to channel number
  //   if(memory[MEM_MIDIOUT_CC_SCALING+channel]) { //CC SCALING to channel number
  //     v = (v & 0x0F)*8;
  //   }
  //   value = (channel*7) + ((v>>4) & 0x07);
  //   MIDI.sendControlChange(memory[MEM_MIDIOUT_CC_NUMBERS+value], v, channel+1);
  // } 
  // else {
  //   if(memory[MEM_MIDIOUT_CC_SCALING+channel]) {
  //     float s;
  //     s = value;
  //     v = ((s / 0x6f) * 0x7f);
  //   }
  //   value = (channel*7);
  //   MIDI.sendControlChange(memory[MEM_MIDIOUT_CC_NUMBERS+value], v, channel+1);
  // }
}

void playPC(byte channel, byte value)
{
  // MIDI.sendProgramChange(value, memory[MEM_MIDIOUT_NOTE_CH+m]+1);
}

void midioutDoAction(byte command, byte value) //m is midi channel or command, v is midi value
{
  // if(command < 0x04) {// note message not sure where is the reference but when is < 4 it is considered as channel
  //   if(value) {
  //     checkStopNote(command);
  //     playNote(command, value);
  //   } 
  //   else if (midiOutLastNote[m]>=0) {
  //     stopNote(m);
  //   }
  // } 
  // else if (command < 0x08) { // control change message deducted by 4 resulting channel 0-3
  //   byte channel = command - 4;
  //   //cc message
  //   playCC(channel, value);
  // } 
  // else if(command < 0x0C) { // program change message deducted by 8 resulting channel 0-3
  //   byte channel = command - 8;
  //   playPC(channel, value);
  // }
}

void stopNote(byte channel)//channel index 0-3 represent midi channel 1-4
{
  for(int countHoldNote = 0; countHoldNote < midioutNoteHoldCounter[channel]; countHoldNote++) {
    MIDI.sendNoteOff(midioutNoteHold[channel][countHoldNote], 0, channel+1);
  }
  midiOutLastNote[channel] = -1;
  midioutNoteHoldCounter[channel] = 0;
}

void stopAllNotes()
{
  //stop all notes in each channel
  for(int channel = 0; channel < 4; channel++) { //m is channel reserved 0-3: channel 1-4
    if(midiOutLastNote[channel] >= 0) { //set to -1 when there is no last note played in the channel
      stopNote(channel);
    }
    MIDI.sendControlChange(123, 0x7F, channel+1); //123 is all notes off, 0x7F is velocity
  }
}


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
            Serial.println("clock");
            //calculateBPM();
            break;
          case 0x7E: //seq stop
            MIDI.sendRealTime(midi::Stop);
            stopAllNotes(); //not sure needed yet
            Serial.println("stop");
            break;
          case 0x7D: //seq start
            MIDI.sendRealTime(midi::Start);
            Serial.println("start");
            break;
          default:
            midiData[0] = (incomingMidiByte - 0x70); //command??
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

  Serial.begin(31250);
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