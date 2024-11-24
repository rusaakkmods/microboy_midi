#include <MIDIUSB.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <limits.h>

#define PRODUCT_NAME "MicroBOY MIDI"
#define VERSION "v0.0.1"
#define RUSAAKKMODS "rusaaKKMODS @ 2024"

// #define DEBUG_MODE

// #define USE_MIDI_h  // comment this line if you want to use USB MIDI

#ifdef USE_MIDI_h
#include <MIDI.h>
MIDI_CREATE_INSTANCE(HardwareSerial, Serial1, MIDI);
#else
HardwareSerial &MIDI = Serial1;
#endif

// RESERVERD PINS
// #define OLED_SDA SDA
// #define OLED_SCL SCL
#define CLOCK_PIN A0 // to be connected to Gameboy Clock
#define SO_PIN A1    // to be connected to Gameboy Serial In
#define SI_PIN A2    // to be connected to Gameboy Serial Out

// rotary encoder for control and settings
#define ROTARY_CLK 9
#define ROTARY_DT 8
#define ROTARY_SW 7

// tweak these value to get better stability
#define BIT_DELAY 1
#define BYTE_DELAY 4000
#define MIDI_DELAY 20000

// OLED display
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32
#define OLED_I2C_ADDRESS 0x3C

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

byte outputChannel[4] = {1, 2, 3, 4};         // default channel 1, 2, 3, 4 //TODO: controlable by rotary encoder and save in EEPROM
bool ccMode[4] = {true, true, true, true};    // default CC mode 1, 1, 1, 1 //TODO: should be configurable maybe save in EEPROM
bool ccScaling[4] = {true, true, true, true}; // default scaling 1, 1, 1, 1 //TODO: should be configurable maybe save in EEPROM
byte ccNumbers[4][7] = {
    {1, 2, 3, 7, 10, 11, 12},
    {1, 2, 3, 7, 10, 11, 12},
    {1, 2, 3, 7, 10, 11, 12},
    {1, 2, 3, 7, 10, 11, 12}}; // default CC numbers 1, 2, 3, 7, 10, 11, 12 //TODO: should be configurable maybe save in EEPROM

// enable these features, caution! it is GLITCHY!!
bool pcEnabled = false;
bool ccEnabled = false;
bool realTimeEnabled = false;

// Clocking
uint64_t lastTapTime = 0;
volatile float interval = 0.00;
uint64_t nextClockTime = 0;
float bpm = 0.00;
uint8_t ticking = 0;
uint8_t tickBuffer = 0.01;

// Gameboy Serial Communication
uint64_t lastReadGameboy = 0;
bool isCommandWaiting = false;
byte commandWaiting = 0x00;
byte experimentalCorrection = 0x00;

// track 0-PU1, 1-PU2, 2-WAV, 3-NOI
volatile byte lastNote[4] = {0, 0, 0, 0};
volatile int velocity = 127; // adjustable Global Velocity, affecting all channels
volatile byte lastTrack = 0; // Track with the shortest update interval

// control interface
byte switchControlState;
bool lastControlState = HIGH;
bool controlPressed = false;
uint64_t lastPrint = 0;

void sendMIDI(midiEventPacket_t message)
{
  MidiUSB.sendMIDI(message);
  MidiUSB.flush();

#ifndef USE_MIDI_h
  Serial1.write(message.byte1);
  Serial1.write(message.byte2);
  Serial1.write(message.byte3);
  Serial1.flush();
#endif
}

void noteOn(byte track, byte note)
{
  noteStop(track); // stop previous note consider each track is monophonics
  // send to USB MIDI
  midiEventPacket_t message = {0x09, 0x90 | outputChannel[track] - 1, note, velocity};
  sendMIDI(message);

#ifdef USE_MIDI_h
  MIDI.sendNoteOn(note, velocity, outputChannel[track]);
  Serial1.flush();
#endif

  lastNote[track] = note;
}

void noteStop(byte track)
{
  byte note = lastNote[track];
  if (note)
  {
    // send to USB MIDI
    midiEventPacket_t message = {0x08, 0x80 | outputChannel[track] - 1, note, 0x00};
    sendMIDI(message);

#ifdef USE_MIDI_h
    MIDI.sendNoteOff(note, 0x00, outputChannel[track]);
    Serial1.flush();
#endif

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

void sendNote(byte track, byte note)
{
  if (note)
  { // value > 0 then its a "Note On" 1-127 | LSDJ note range 0-119
    noteOn(track, note);
  }
  else
  { // value 0 then its a "Note Off"
    noteStop(track);
  }
}

void clockReset()
{
  ticking = 0;
  lastTapTime = 0;
  interval = 0;
  bpm = 0;
  nextClockTime = 0;
}

void clockMIDI()
{ // its clocking time!!
  if (bpm > 0)
  {
    unsigned long currentTime = micros();
    if (currentTime >= nextClockTime)
    {
      midiEventPacket_t message = {0x0F, 0xF8, 0x00}; // MIDI Clock
      sendMIDI(message);

#ifdef USE_MIDI_h
      MIDI.sendRealTime(midi::Clock);
      Serial1.flush();
#endif

      // float bufferedInterval = interval * (1 - tickBuffer); //buffer: to make sure executed well
      // Calculate the tick interval based on the current BPM
      // unsigned long tickInterval = 60000000 / (bpm * 24);

      nextClockTime = currentTime + interval;
    }
  }
}

void clockTap()
{
  unsigned long currentTime = micros();

  if (lastTapTime > 0) // skip on first tap
  {
    uint64_t currentInterval = round((currentTime - lastTapTime) / 6);
    interval = currentInterval;

    bpm = round((60000000.0 / (interval * 24)) / 5) * 5;
  }
  else
  {
    // kicked off starter!!!
    bpm = 120;
    interval = 20833.33;
  }
  lastTapTime = currentTime;

  // bpm = 120;
  // interval = 20833.33;
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

  // send to USB MIDI
  midiEventPacket_t message = {0xB0 | outputChannel[track] - 1, ccNumber, ccValue};
  sendMIDI(message);

#ifdef USE_MIDI_h
  MIDI.sendControlChange(ccNumber, ccValue, outputChannel[track]);
  Serial1.flush();
#endif

#ifdef DEBUG_MODE
  Serial.print("CC Command - track:");
  Serial.print(track);
  Serial.print(" value:");
  Serial.println(value);
#endif
}

void sendProgramChange(byte track, byte value)
{
  // send to USB MIDI
  midiEventPacket_t message = {0xC0 | outputChannel[track] - 1, value, 0x00};
  sendMIDI(message);

#ifdef USE_MIDI_h
  MIDI.sendProgramChange(value, outputChannel[track]);
  Serial1.flush();
#endif
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
    sendNote(track, value);
  }
  else if (command < 0x08)
  { // 4-7 represent CC message
    track = command - 0x04;
    if (ccEnabled)
      sendControlChange(track, value);
#ifdef DEBUG_MODE
    Serial.println("Control Change!");
#endif
  }
  else if (command < 0x0C)
  { // 8-11 represent PC message
    track = command - 0x08;
    if (value == 0x7F) // let's use the GUNSHOT!!! y-FF command!
    {
      if (realTimeEnabled)
        clockTap();
    }
    else
    {
      if (pcEnabled)
        sendProgramChange(track, value);
#ifdef DEBUG_MODE
      Serial.println("Program Change!");
#endif
    }
  }
  else if (command <= 0x0F)
  { // 12-15 not yet sure!
    track = command - 0x0C;
    // unknown! skip consume one value byte usually 0 or 127
  }
  else
  {
    return; // not supposed to happened!!
  }
  lastTrack = track;
}

void routeRealtime(byte command)
{
  if (!realTimeEnabled)
    return;

  midiEventPacket_t message;
  switch (command)
  {
  case 0x7D: // send start to USB MIDI
    message = {0xFA};
    sendMIDI(message);

#ifdef USE_MIDI_h
    MIDI.sendRealTime(midi::Start);
    Serial1.flush();
#endif
    clockReset();
#ifdef DEBUG_MODE
    Serial.println("Start");
#endif
    break;
  case 0x7E: // send stop to USB MIDI //todo avoid stop glitch
    message = {0xFC};
    sendMIDI(message);

#ifdef USE_MIDI_h
    MIDI.sendRealTime(midi::Stop);
    Serial1.flush();
#endif
    clockReset();
    noteStopAll();

#ifdef DEBUG_MODE
    Serial.println("Stop!");
#endif
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

// byte readIncomingByte()
// {
//   byte receivedByte = 0;
//   PORTF |= (1 << PF6); // making sure Gameboy Serial In is HIGH! explanation: docs/references/gb_link_serial_in.md
//   for (uint8_t i = 0; i < 8; i++)
//   {
//     PORTF |= (1 << PF7); // Set HIGH
//     delayMicroseconds(BIT_DELAY);

//     receivedByte = (receivedByte << 1) + ((PINF & (1 << PINF5)) ? 1 : 0); // Read a bit, and shift it into the byte
//     //if (i == 0 && receivedByte == 0) return 0x7F;

//     PORTF &= ~(1 << PF7); // Set LOW
//     delayMicroseconds(BIT_DELAY);
//   }
//   delayMicroseconds(MIDI_DELAY);
//   return receivedByte &= 0x7F; // Set the MSB range value to 0-127
// }

/* This one based on Arduinoboy version, it is more stable on higher tempo
// Arduinoboy (https://github.com/trash80/Arduinoboy)
// by Timothy Lamb @trash80
*/
byte readIncomingByte()
{
  byte incomingMidiByte;
  delayMicroseconds(BYTE_DELAY);
  PORTF &= ~(1 << PF7); // LOW CLOCK_PIN
  delayMicroseconds(BYTE_DELAY);
  PORTF |= (1 << PF7); // HIGH CLOCK_PIN
  delayMicroseconds(BIT_DELAY);
  if (((PINF & (1 << PINF5)) ? 1 : 0))
  {
    incomingMidiByte = 0;
    for (int i = 0; i != 7; i++)
    {
      PORTF &= ~(1 << PF7); // LOW CLOCK_PIN
      delayMicroseconds(BIT_DELAY);
      PORTF |= (1 << PF7); // HIGH CLOCK_PIN
      incomingMidiByte = (incomingMidiByte << 1) + ((PINF & (1 << PINF5)) ? 1 : 0);
    }
    return incomingMidiByte;
  }
  return 0x7F;
}

void readGameboy()
{
  // enable this for non-blocking read
  // uint64_t currentTime = micros();
  // if (currentTime - lastReadGameboy < MIDI_DELAY) return;

  byte value = readIncomingByte();
  lastReadGameboy = micros();

  if (isCommandWaiting)
  {
    routeMessage(commandWaiting, value);
    isCommandWaiting = false;

    experimentalCorrection = commandWaiting;
    commandWaiting = 0x00;
  }
  else if (value >= 0x7D)
  { // 125-127 realtime message
    routeRealtime(value);
  }
  else if (value >= 0x70)
  { // 112-124 command message
    isCommandWaiting = true;
    commandWaiting = value;
  }
  else
  { // 0 - 111 Hiccups!!! not supposed to happened!!
#ifdef DEBUG_MODE
    if (value == 0)
    {
      Serial.print("Hiccups! Off:");
    }
    else if (value > 0x0F)
    {
      Serial.print("Hiccups! Note:");
    }
    else
    {
      Serial.print("Hiccups! Command:");
    }
    Serial.println(value);
#endif
    // EXPERIMENTAL HICCUPS! CORRECTION!! LOL
    if (experimentalCorrection)
    { // use the last command
      routeMessage(experimentalCorrection, value);
      experimentalCorrection = 0x00;
    }
    else
    { // use the last track
      routeMessage(lastTrack + 0x70, value);
    }
  }
}

ISR(PCINT0_vect)
{
  bool pinState = PINB & (1 << PB5);
  if (lastControlState == HIGH && pinState == LOW)
  {
    if (digitalRead(ROTARY_DT))
    {
      velocity++;
    }
    else
    {
      velocity--;
    }
    velocity = constrain(velocity, 0, 127);
  }
  lastControlState = pinState;
}

void readControl()
{
  switchControlState = digitalRead(ROTARY_SW);
  if (switchControlState == LOW && !controlPressed)
  {
    velocity = 0;
    controlPressed = true;
  }

  if (switchControlState == HIGH)
  {
    controlPressed = false;
  }

  displayMain();
}

void flushSerial()
{
  Serial1.flush();
  MidiUSB.flush();
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

void setup()
{
  // Initialize Pins
  pinMode(CLOCK_PIN, OUTPUT);
  pinMode(SI_PIN, INPUT);
  pinMode(SO_PIN, OUTPUT);
  pinMode(ROTARY_CLK, INPUT_PULLUP);
  pinMode(ROTARY_DT, INPUT);
  pinMode(ROTARY_SW, INPUT_PULLUP);

  PCICR |= (1 << PCIE0);   // Enable pin change interrupt for PCIE0 (Port B)
  PCMSK0 |= (1 << PCINT5); // Enable pin change interrupt for PB5 (PCINT5)

  // IMPORTANT! Gameboy Serial In must be HIGH explanation: docs/references/gb_link_serial_in.md
  digitalWrite(SO_PIN, HIGH);

#ifdef DEBUG_MODE
  Serial.begin(9600); // to serial monitor
#else
  Serial.begin(115200); // to to USB
#endif
  // Initialize MIDI Serial Communication
  Serial1.begin(31250);

#ifdef USE_MIDI_h
  MIDI.begin(MIDI_CHANNEL_OMNI);
#endif

  // Initialize I2C for OLED
  Wire.begin();
  display.begin(SSD1306_SWITCHCAPVCC, OLED_I2C_ADDRESS);
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  clockReset();
  flushSerial();

  displaySplash();
}

void loop()
{
  readControl();
  readGameboy();
  // clockMIDI();
}