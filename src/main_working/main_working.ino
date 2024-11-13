
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <MIDI.h> 

#define PRODUCT_NAME "MEGABOY MIDI"
#define VERSION "v0.0.1"

MIDI_CREATE_INSTANCE(HardwareSerial, Serial1, MIDI);

//MEGA 2560
#define GB_CLOCK_PIN 22
#define GB_SO_PIN 23
#define GB_SI_PIN 24
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
byte isClockMuted = false;

unsigned int channelPulse1 = 1;
unsigned int channelPulse2 = 2;
unsigned int channelWave = 3;
unsigned int channelNoise = 4;

void printDisplay(int mode) {
    display.clearDisplay();
    display.setCursor(0, 0);
    display.print("MEGABOY MIDI");
    display.setCursor(0, 10);
    display.print("MODE: ");
    String activeMode;
    switch (mode) {
        case 3: activeMode = "MI.OUT]"; break;
        case 2: activeMode = "MI.MAP"; break;
        case 1: activeMode = "LSDJ (Master)"; break;
        case 0:
        default: activeMode = "MIDI (Slave)"; break;
    }
    display.print(activeMode);
    display.display();
}

// Generate digital clock pulse
// Generate digital clock pulse to gameboy
// Basically sending 0101010101010101 each pulse
// based on Arduinoboy https://github.com/trash80/Arduinoboy/blob/master/Arduinoboy/Mode_LSDJ_SlaveSync.ino
// by @trash80
void sendClockPulse() {
  if (!isClockMuted) {
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
        MIDI.sendStart();
        sequencerStarted = true;
      }
      MIDI.sendClock();
      ticks = 0;
    }
    ticks++;
    if(ticks == 3) ticks = 0; // somehow 3 works instead of 8bit!! 
}

// This code is based on Arduinoboy: https://github.com/trash80/Arduinoboy/blob/master/Arduinoboy/Mode_LSDJ_MasterSync.ino by @trash80
// Reading Clock Input from Gameboy detecting sequencer play and stop.
// Bypass "note on" message for active LSDJ row/channel song position
void masterReading() {
  readClockLine = digitalRead(GB_CLOCK_PIN); 
  if (readClockLine) {
    do { // read until false
      countPulse++;
      if(sequencerStarted && countPulse > 16000) { // detect if lsdj sequencer stopped
        countPulse = 0;
        MIDI.sendStop();
        sequencerStarted = false; 
      }
    } while (digitalRead(GB_CLOCK_PIN));
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
  MIDI.setHandleClock(mutePulseClock); // handle clock message
  MIDI.setHandleStart(mutePulseClock); // handle start message
  MIDI.setHandleContinue(mutePulseClock); // handle continue message
  MIDI.setHandleStop(mutePulseClock); // handle stop message

  do 
  {
    masterReading();
  } while (modeReading() == MODE_MASTER);
}

void modeSwitch() {
  int mode = modeReading();
  printDisplay(mode);
  switch(mode) {
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
  MIDI.sendStop();
}

void setup() {
  pinMode(MODE_KNOB_PIN, INPUT);

  pinMode(GB_SI_PIN, INPUT);
  pinMode(GB_SO_PIN, OUTPUT);

  //Serial.begin(9600);
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