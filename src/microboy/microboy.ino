#include <MIDI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <avr/io.h>
#include <avr/interrupt.h>

#define PRODUCT_NAME "MicroBOY MIDI"
#define VERSION "v0.0.1"
#define RUSAAKKMODS "rusaaKKMODS @ 2024"

#define DEBUG_MODE

// RESERVERD PINS
// #define OLED_SDA SDA
// #define OLED_SCL SCL
#define STATUS_PIN LED_BUILTIN
#define CLOCK_PIN 15 // F7 to be connected to Gameboy Clock      //PB1
#define SO_PIN 16    // F6 to be connected to Gameboy Serial In  //PB2
#define SI_PIN 14    // F5 to be connected to Gameboy Serial Out //PB3
#define VEL_KNOB_PIN A10

// tweak these value to get better stability, lower value will give better stability but slower
#define CLOCK_DELAY 1
#define BYTE_DELAY 2000
// #define SC_PERIOD 125                      // Gameboy Clock period in microseconds (approx. 8 kHz) !not yet sure it's necessary
// #define PPQN 24                            // Pulses Per Quarter Note (PPQN) for MIDI clock

// OLED display
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32
#define OLED_I2C_ADDRESS 0x3C

MIDI_CREATE_INSTANCE(HardwareSerial, Serial1, MIDI);

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);


byte outputChannel[4] = {1, 2, 3, 4};         // default channel 1, 2, 3, 4 //TODO: controlable by rotary encoder and save in EEPROM
bool ccMode[4] = {true, true, true, true};    // default CC mode 1, 1, 1, 1 //TODO: should be configurable maybe save in EEPROM
bool ccScaling[4] = {true, true, true, true}; // default scaling 1, 1, 1, 1 //TODO: should be configurable maybe save in EEPROM
byte ccNumbers[4][7] = {
    {1, 2, 3, 7, 10, 11, 12},
    {1, 2, 3, 7, 10, 11, 12},
    {1, 2, 3, 7, 10, 11, 12},
    {1, 2, 3, 7, 10, 11, 12}}; // default CC numbers 1, 2, 3, 7, 10, 11, 12 //TODO: should be configurable maybe save in EEPROM

uint64_t lastPrint = 0;     // last time the display was updated
uint64_t lastClockTime = 0; // Time of the last MIDI clock message
uint16_t bpm = 0;           // Calculated BPM

volatile byte lastNote[4] = {0, 0, 0, 0}; // track 0-PU1, 1-PU2, 2-WAV, 3-NOI
volatile byte velocity = 0;               // adjustable Global Velocity, affecting all channels
volatile byte lastTrack = 0;              // Track with the shortest update interval

void calculateBPM()
{
  uint64_t currentTime = millis();
  uint64_t elapsedTime = currentTime - lastClockTime;
  bpm = 60000 / (elapsedTime * 4);
  lastClockTime = currentTime;
}

void notePlay(byte track, byte note)
{
  noteStop(track); // stop previous note consider each track is monophonics
  MIDI.sendNoteOn(note, velocity, outputChannel[track]);
  lastNote[track] = note;
}

void noteStop(byte track)
{
  byte note = lastNote[track];
  if (note)
  {
    MIDI.sendNoteOff(note, 0x00, outputChannel[track]);
    lastNote[track] = 0;
  }
}

void noteStopAll()
{
  for (uint8_t track = 0; track < 4; track++)
  {
    noteStop(track);
  }
}

void sendNotes(byte track, byte note)
{
  if (note)
  {                        // value > 0 then its a "Note On" 1-127 | LSDJ note range 0-119
    notePlay(track, note); // trigger the note
  }
  else
  {                  // value 0 then its a "Note Off"
    noteStop(track); // stop all note in channel
  }
}

void sendClock()
{
  digitalWrite(STATUS_PIN, HIGH);
  // Send 6 MIDI clock messages LSDJ step per step to simulate 24 PPQN
  for (uint8_t tick = 0; tick < 6; tick++)
  {
    MIDI.sendRealTime(midi::Clock);
    delayMicroseconds(CLOCK_DELAY); // give a very small delay
  }
  calculateBPM();
  digitalWrite(STATUS_PIN, LOW);
}

/* Reference from Arduinoboy (https://github.com/trash80/Arduinoboy)
// by Timothy Lamb @trash80
// ------------------------------------
// X## - Sends a MIDI CC -
// By default in Arduinoboy the high nibble selects a CC#, and the low nibble sends a value 0-F to 0-127.
// This can be changed to allow just 1 midi CC with a range of 00-6F, or 7 CCs with scaled or unscaled values.
//
// CC Mode:
// default: 1 for all 4 tracks
// - 0: use 1 midi CC, with the range of 00-6F,
// - 1: uses 7 midi CCs with the range of 0-F (the command's first digit would be the CC#)
// either way the value is scaled to 0-127 on output
// ------------------------------------
// CC Scaling:
// default: 1 for all 4 tracks
// - 1: scales the CC value range to 0-127 as oppose to lsdj's incomming 00-6F (0-112) or 0-F (0-15)
// - 0: no scaling, the value is directly sent to midi out 0-112 or 0-15
// scaling in c:
// byte scaledValue = map(value, 0, 112, 0, 127); // 0-112 to 0-127
// byte scaledValue = map(value, 0, 15, 0, 127); // 0-112 to 0-127
// ------------------------------------
// CC Message Numbers
// default options: {1,2,3,7,10,11,12} for each track, these options are CC numbers for lsdj midi out
// If CC Mode is 1, all 7 ccs options are used per channel at the cost of a limited resolution of 0-F
*/
void sendControlChange(byte track, byte value)
{
  byte ccNumber;
  byte ccValue;

  if (ccMode[track])
  {
    if (ccScaling[track])
    {
      // CC Mode 1 with scaling
      ccNumber = ccNumbers[track][(value & 0xF0) >> 4]; // High nibble
      ccValue = map(value & 0x0F, 0, 15, 0, 127);       // Low nibble
    }
    else
    {
      // CC Mode 1 without scaling
      ccNumber = ccNumbers[track][(value & 0xF0) >> 4]; // High nibble
      ccValue = value & 0x0F;                           // Low nibble
    }
  }
  else
  {
    if (ccScaling[track])
    {
      // CC Mode 0 with scaling
      ccNumber = ccNumbers[track][0];       // Use the first CC number for Mode 0
      ccValue = map(value, 0, 112, 0, 127); // Scale the value
    }
    else
    {
      // CC Mode 0 without scaling
      ccNumber = ccNumbers[track][0]; // Use the first CC number for Mode 0
      ccValue = value;                // Direct value
    }
  }

  MIDI.sendControlChange(ccNumber, ccValue, outputChannel[track]);

#ifdef DEBUG_MODE
  Serial.print("CC Command - track:");
  Serial.print(track);
  Serial.print(" value:");
  Serial.println(value);
#endif
}

void sendProgramChange(byte track, byte value)
{
  MIDI.sendProgramChange(value, outputChannel[track]);

#ifdef DEBUG_MODE
  Serial.print("PC Command - track:");
  Serial.print(track);
  Serial.print(" value:");
  Serial.println(value);
#endif
}

void routeMessage(byte message, byte value)
{
  byte command = message - 0x70;
  byte track = 0;
  if (command < 0x04)
  { // 0-3 represent Track index
    track = command;
    sendNotes(track, value);
  }
  else if (command < 0x08)
  { // 4-7 represent CC message
    track = command - 0x04;
    //sendControlChange(track, value);
  }
  else if (command < 0x0C)
  { // 8-11 represent PC message
    track = command - 0x08;
    if (value == 0x7F) // let's use the GUNSHOT!!!
    {
      /*
      // Redirect LSDJ "Y-FF" effect command to trigger beat clock counter
      // -----------------------------------------------------------------
      // thanks to @nikitabogdan & instagram: alfian_nay93 for the inspiring idea visit: https://github.com/nikitabogdan/arduinoboy
      // Utilizing PC Message!!
      //
      // Each YFF command will send MIDI clock signal
      // usage:
      // >> simply put a Y-FF command in a table step-1 follow by H-00 command in step-2 for normal measurement 4/4 and step-3 for 3/4 triplets
      */
      sendClock();
    }
    else
    {
      //sendProgramChange(track, value);
    }
  }
  else if (command <= 0x0F)
  { // 12-15 not yet sure!
    track = command - 0x0C;
    // unknown! skip consume one value byte usually 0 or 127
  }
  else
  {
    // not supposed to happened!!
    return;
  }
  lastTrack = track;
}

void routeRealtime(byte command)
{
  switch (command)
  {
  case 0x7D:
    MIDI.sendRealTime(midi::Start);
    break;
  case 0x7E:
    MIDI.sendRealTime(midi::Stop);
    noteStopAll();
    break;
  case 0x7F:
    // microboy byte reading clock!! ignore for now.. very missleading....
    break;
  default:
#ifdef DEBUG_MODE
    Serial.print("Unknown Realtime: ");
    Serial.print(command);
#endif
    break;
  }
}

byte readIncomingByte()
{
  byte receivedByte = 0;
  for (uint8_t i = 0; i < 8; i++)
  {
    // use PORT instead of digitalWrite to reduce delay
    PORTB |= (1 << PB1); // Set PORTF7 HIGH CLOCK_PIN 
    //digitalWrite(CLOCK_PIN, HIGH);
    delayMicroseconds(CLOCK_DELAY / 2);

    receivedByte = (receivedByte << 1) + ((PINB & (1 << PINB3)) ? 1 : 0); // Read the bit from Gameboy Serial Out on SI_PIN

    PORTB &= ~(1 << PB1); // Set PORTF7 LOW
    //digitalWrite(CLOCK_PIN, LOW);
    delayMicroseconds(CLOCK_DELAY / 2);
  }
  // TODO: 
  // need to eliminate this delay in the future, millis/micros doesn't work well. 
  // I'm thinking other board implementation: pico for multithreading, freeRTOS!! but naahh!! I'll try using interrupt first!
  delayMicroseconds(BYTE_DELAY); // Small delay to allow Game Boy to prepare for the next byte transmission
  return receivedByte &= 0x7F; // Set the MSB range value to 0-127
}

void readGameboy()
{
  byte command = readIncomingByte();
  if (command >= 0x7D)
  { // 125-127 realtime message
    // todo: only enabled when clock detected
    routeRealtime(command);
  }
  else if (command >= 0x70)
  { // 112-124 command message
    byte value = readIncomingByte(); // next byte should be the value
    routeMessage(command, value);
  }
  else
  { // 0 - 111 Hiccups!!! not supposed to happened!!
#ifdef DEBUG_MODE
    // reason could be:
    // 1. Unstable gameboy clock
    // 2. Unstable gameboy cpu
    // 3. Unstable microcontroller
    // 4. Unstable Gameboy power issue
    // 5. Gameboy link cable issue
    // 6. Gameboy serial communication issue
    if (command == 0)
    {
      Serial.print("Hiccups! Off:");
    }
    else
    {
      if (command > 0x0F)
      {
        Serial.print("Hiccups! Note:");
      }
      else
      {
        Serial.print("Hiccups! Command:");
      }
    }
    Serial.println(command);
#endif

    // EXPERIMENTAL HICCUPS! CORRECTION!! LOL
    routeMessage(lastTrack + 0x70, command); // re-route message with the hiccups command
  }
}

void readControl()
{
  uint16_t knobValue = analogRead(VEL_KNOB_PIN);
  velocity = map(knobValue, 0, 1023, 0, 127);

  displayMain();
}

void displaySplash()
{
  display.clearDisplay();
  display.setCursor(0, 0);
  display.println(PRODUCT_NAME);
  display.setCursor(0, 10);
  display.println(VERSION);
  display.setCursor(0, 20);
  display.print(RUSAAKKMODS);
  display.display();
}

void displayMain()
{
  uint64_t currentTime = millis();
  if (currentTime - lastPrint >= 1000)
  { // update display every 1 second
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

ISR(PCINT0_vect) {
  Serial.println("Interrupt");
}

void setup()
{
  // Initialize Pins
  pinMode(STATUS_PIN, OUTPUT);
  pinMode(CLOCK_PIN, OUTPUT);
  pinMode(SI_PIN, INPUT);
  pinMode(SO_PIN, INPUT);
  pinMode(VEL_KNOB_PIN, INPUT);

  // Enable pin change interrupt on PB3 (PCINT3)
  PCICR |= (1 << PCIE0); // Enable pin change interrupt for PCINT[7:0]
  PCMSK0 |= (1 << PCINT3); // Enable pin change interrupt for PB3

  sei(); // Enable global interrupts

  digitalWrite(SO_PIN, LOW);
  digitalWrite(SI_PIN, LOW);

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

void loop()
{
  readControl();
  readGameboy();
}