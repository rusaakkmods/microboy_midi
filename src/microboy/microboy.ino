#include <MIDI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define PRODUCT_NAME "MICROBOY MIDI"
#define VERSION "v0.0.1"

MIDI_CREATE_INSTANCE(HardwareSerial, Serial1, MIDI);

// OLED display
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32
#define OLED_I2C_ADDRESS 0x3C

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

#define CLOCK_PIN A0
#define SI_PIN A2
#define SO_PIN A1

//#define SC_PERIOD 125                     // Gameboy Clock period in microseconds (approx. 8 kHz) !not yet sure it's necessary
#define CLOCK_DELAY 2                       // microseconds between 1-63 microseconds is recommended
#define BYTE_DELAY 8000                     // microseconds between 600-10000 microseconds is recommended

// Control interfaces
#define BPM_KNOB_PIN A10                    // Use PWM pin for analog input
#define PPQN 24                             // Pulses Per Quarter Note (PPQN) for MIDI clock

#define VELOCITY 100                        // TODO: controlable velocity by knob
                                            // THOUGHT! gated effect???

volatile byte lastNote[4] = {0, 0, 0, 0};   // track 0-PU1, 1-PU2, 2-WAV, 3-NOI
byte outputChannel[4] = {1, 2, 3, 4};       // default channel 1, 2, 3, 4 //TODO: controlable by rotary encoder

unsigned long lastPrint = 0;                // last time the display was updated

volatile int bpm;
volatile bool runClock = false;
unsigned long lastTick = 0;  

void playNote(byte track, byte note) {
    stopNote(track); // stop previous note consider each track is monophonics
    MIDI.sendNoteOn(note, VELOCITY, outputChannel[track]);
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

void sendCommand(byte message) {
  byte data = readIncommingByte();
  
  byte track = 0;
  if (message < 0x04) { // 0-3 represent Track index
    track = message;
    if (data) { // value > 0 then its a "Note On" 1-127
      playNote(track, data); // trigger the note
    }
    else { // value 0 then its a "Note Off"
      stopNote(track); // stop all note in channel
    }
  }
  else if (message < 0x08) { // 4-7 represent CC message
    track = message - 0x04;
    //todo CC message
  }
  else if (message < 0x0C) { // 8-11 represent PC message
    track = message - 0x08;
    //todo PC message
  }
  else {
    return; // unknown message
  }
}

void startManualClock() {
  runClock = true;
}

void stopManualClock() {
  runClock = false;
}

void sendMessage(byte message) {
  switch (message)
  {
    case 0x7D:
      startManualClock();
      MIDI.sendRealTime(midi::Start);
      break;
    case 0x7E:
      stopManualClock();
      MIDI.sendRealTime(midi::Stop);
      stopAll();
      break;
    case 0x7F:
      // IGNORE! false tick! since Gameboy is Slave
      break;
    default:
      sendCommand(message - 0x70); // command message 0-15
      break;
  }
}

byte readIncommingByte() {
  byte receivedByte = 0;
  
  for (int i = 0; i < 8; i++) {
    digitalWrite(CLOCK_PIN, HIGH); // Send HIGH Signal to Gameboy Clock
    delayMicroseconds(CLOCK_DELAY);

    receivedByte = (receivedByte << 1) + digitalRead(SI_PIN); // Read the bit from Gameboy Serial Out

    digitalWrite(CLOCK_PIN, LOW);
    delayMicroseconds(CLOCK_DELAY);
  }

  delayMicroseconds(BYTE_DELAY); // Small delay to allow Game Boy to prepare for the next byte
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
    display.print("BPM:");
    display.print(bpm);
    display.display();
    lastPrint = currentTime;
  }
}

void readControls(){
  unsigned int bpmKnob = analogRead(BPM_KNOB_PIN);
  bpm = map(bpmKnob, 0, 1023, 40, 300); // set BPM range from 40 to 300
  bpm = round(bpm / 20.0) * 20; // Snap to the nearest multiple of 5

  printDisplay();
}

void sendClock() {
  if (runClock) {
    unsigned long interval = (60L * 1000000L) / (bpm * PPQN);
    unsigned long currentTime = micros(); //use micros() for more accurate timing
    if (currentTime - lastTick >= interval) {
      MIDI.sendRealTime(midi::Clock);
      lastTick = currentTime;
    }
  }
}

void setup() {
  // Initialize Pins
  pinMode(CLOCK_PIN, OUTPUT);
  pinMode(SI_PIN, INPUT);
  pinMode(SO_PIN, INPUT);

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
  sendMessage(readIncommingByte());
  //readControls();
  //sendClock();

}