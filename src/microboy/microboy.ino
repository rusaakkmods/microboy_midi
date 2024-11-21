#include <MIDI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <limits.h>

#define PRODUCT_NAME "MICROBOY MIDI"
#define VERSION "v0.0.1"

#define DEBUG_MODE

MIDI_CREATE_INSTANCE(HardwareSerial, Serial1, MIDI);

// OLED display
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32
#define OLED_I2C_ADDRESS 0x3C

#define BUFFER_SIZE 32 // 6 - 12 recomended

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

#define CLOCK_PIN A0
#define SI_PIN A2
#define SO_PIN A1

//#define SC_PERIOD 125                     // Gameboy Clock period in microseconds (approx. 8 kHz) !not yet sure it's necessary
#define CLOCK_DELAY 1                       // microseconds between 1-122 microseconds is recommended
#define BYTE_DELAY 2000                     // microseconds between 600-1000 microseconds is recommended

// Control interfaces
#define VEL_KNOB_PIN A10                    // Use PWM pin for analog input
#define PPQN 24                             // Pulses Per Quarter Note (PPQN) for MIDI clock

#define VELOCITY 127                          // TODO: controlable velocity by knob

volatile byte lastNote[4] = {0, 0, 0, 0};   // track 0-PU1, 1-PU2, 2-WAV, 3-NOI
byte outputChannel[4] = {1, 2, 3, 4};       // default channel 1, 2, 3, 4 //TODO: controlable by rotary encoder

unsigned long lastPrint = 0;                // last time the display was updated

volatile byte lastTrack = 0; // Track with the shortest update interval
volatile int bpm = 0; // Calculated BPM

volatile byte velocity = 0; 

void playNote(byte track, byte note) {
    stopNote(track); // stop previous note consider each track is monophonics
    MIDI.sendNoteOn(note, velocity, outputChannel[track]);
    lastNote[track] = note;
}

void stopNote(byte track) {

  byte note = lastNote[track];
  if (note) {
    MIDI.sendNoteOff(note, 0x00, outputChannel[track]);
    lastNote[track] = 0;
  }
}

void stopAll() {
  for (int ch = 0; ch < 4; ch++) {
    stopNote(ch);
  }
}

void sendMessage(byte message, byte value) {
  byte command = message - 0x70;
  byte track = 0;
  if (command < 0x04) { // 0-3 represent Track index
    track = command;
    if (value) { // value > 0 then its a "Note On" 1-127 | LSDJ note range 0-119
      playNote(track, value); // trigger the note
    }
    else { // value 0 then its a "Note Off"
      stopNote(track); // stop all note in channel
    }
  }
  else if (command < 0x08) { // 4-7 represent CC message
    track = command - 0x04;
    //todo CC message
     stopNote(track); // stop all note in channel
  }
  else if (command < 0x0C) { // 8-11 represent PC message
    track = command - 0x08;
    //todo PC message
  }
  else if (command <= 0x0F) { // 12-15 not yet sure!
    track = command - 0x0C;
    //unknown! skip consume one value byte usually 0 or 127
  }
  else {
    //not supposed to happened!!
    return;
  }
  lastTrack = track;
}

void sendRealtime(byte command) {
  switch (command)
  {
    case 0x7D:
      MIDI.sendRealTime(midi::Start);
      break;
    case 0x7E:
      MIDI.sendRealTime(midi::Stop);
      stopAll();
      break;
    case 0x7F:
      //microboy byte reading clock!! ignore for now
      break;
    default:
    #ifdef DEBUG_MODE
      Serial.print("Unknown Realtime: ");
      Serial.print(command);
    #endif
      break;
  }
}

uint8_t readIncomingByte() {
  uint8_t receivedByte = 0;
  for (int i = 0; i < 8; i++) {
    PORTF |= (1 << PF7); // Set PORTF7 HIGH CLOCK_PIN
    delayMicroseconds(CLOCK_DELAY / 2);

    receivedByte = (receivedByte << 1) + ((PINF & (1 << PINF5)) ? 1 : 0); // Read the bit from Gameboy Serial Out SI_PIN

    PORTF &= ~(1 << PF7); // Set PORTF7 LOW
    delayMicroseconds(CLOCK_DELAY / 2);
  }
  delayMicroseconds(BYTE_DELAY); // Small delay to allow Game Boy to prepare for the next byte | need to remove this in a future
  return receivedByte &= 0x7F; // Set the MSB range value to 0-127
}

void displaySplash() {
    display.clearDisplay();
    display.setCursor(0, 0);
    display.println(PRODUCT_NAME);
    display.setCursor(0, 10);
    display.println(VERSION);
    display.setCursor(0, 20);
    display.print("rusaaKKMODS @ 2024");
    display.display();
} 

void printDisplay() {
  unsigned long currentTime = millis();
  if (currentTime - lastPrint >= 1000) { //update display every 1 second
    display.clearDisplay();
    display.setCursor(0, 0);
    display.print("PU1 [");
    display.print(String(outputChannel[0] < 10 ? "0" : "") + String(outputChannel[0]));
    display.print("] | PU2 [");
    display.print(String(outputChannel[1] < 10 ? "0" : "") + String(outputChannel[1]));
    display.println("]");
    display.setCursor(0, 10);
    display.print("WAV [");
    display.print(String(outputChannel[2] < 10 ? "0" : "") + String(outputChannel[2]));
    display.print("] | NOI [");
    display.print(String(outputChannel[3] < 10 ? "0" : "") + String(outputChannel[3]));
    display.println("]");
    display.setCursor(0, 20);
    display.print("VEL:");
    display.print(velocity);
    display.print(" BPM:");
    display.print(bpm);
    display.display();
    lastPrint = currentTime;
  }
}

void readControl(){
  int knobValue = analogRead(VEL_KNOB_PIN);
  velocity = map(knobValue, 0, 1023, 0, 127);

  printDisplay();
}

void readGameboy() {
  byte command = readIncomingByte();
  if (command >= 0x7D) { // 125-127 realtime message
    sendRealtime(command);
  }
  else if (command >= 0x70) { // 112-124 command message
    byte value = readIncomingByte(); // next byte should be the value
    sendMessage(command, value);
  }
  else { // 0 - 111
    //experimental correction
    sendMessage(lastTrack + 0x70, command);
    //reason could be: 
    // 1. Unstable gameboy clock
    // 2. Unstable gameboy cpu
    // 3. Unstable microcontroller
    // 4. Unstable Gameboy power issue
    // 5. Gameboy link cable issue
    // 6. Gameboy serial communication issue

#ifdef DEBUG_MODE
    //Hiccup!!
    if (command == 0) {
      Serial.print("Hiccups! Off:");
    }
    else {
      if (command > 0x0F) {
        Serial.print("Hiccups! Note:");
      }
      else {
        Serial.print("Hiccups! Command:");
      }
    }
    Serial.println(command);
#endif

  }
}

void setup() {
  // Initialize Pins
  pinMode(CLOCK_PIN, OUTPUT);
  pinMode(SI_PIN, INPUT);
  pinMode(SO_PIN, INPUT);

  pinMode(VEL_KNOB_PIN, INPUT);

#ifdef DEBUG_MODE
  Serial.begin(9600);
#endif

  // Initialize MIDI Serial Communication
  Serial1.begin(31250);
  MIDI.begin(MIDI_CHANNEL_OMNI);

  // Initialize I2C for OLED
  Wire.begin();
  display.begin(SSD1306_SWITCHCAPVCC, OLED_I2C_ADDRESS);
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  displaySplash();
}

void loop() {
  readControl();
  readGameboy();
}