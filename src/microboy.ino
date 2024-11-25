#include <MIDIUSB.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <limits.h>

#define PRODUCT_NAME "MicroBOY MIDI"
#define VERSION "v0.0.1"
#define RUSAAKKMODS "rusaaKKMODS @ 2024"

// build modes options
// #define DEBUG_MODE               //- enable debug mode
// #define USE_MIDI_h               //- use MIDI Library for MIDI Serial Communication (faster but unstable! buffer overflow in faster rate)
// #define ARDUINOBOY_READER        //- use Arduinoboy version of reader also stable, tweak byteDelay for better stability
#define NON_BLOCKING_DELAY //- use non-blocking read for Gameboy Serial Communication

#ifdef USE_MIDI_h
// MIDI Library for MIDI Serial Communication supposed to be faster but unstable
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

// tweak these value to get better stability, lower value is faster but less stable
#define BIT_DELAY 1

// OLED display
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32
#define OLED_I2C_ADDRESS 0x3C

#define PPQN 24 // Pulses Per Quarter Note

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

uint32_t byteDelay = 4000;                    // TODO: should be configurable maybe save in EEPROM
byte outputChannel[4] = {1, 2, 3, 4};         // default channel 1, 2, 3, 4 //TODO: controlable by rotary encoder and save in EEPROM
bool ccMode[4] = {true, true, true, true};    // default CC mode 1, 1, 1, 1 //TODO: should be configurable maybe save in EEPROM
bool ccScaling[4] = {true, true, true, true}; // default scaling 1, 1, 1, 1 //TODO: should be configurable maybe save in EEPROM
byte ccNumbers[4][7] = {
    {1, 2, 3, 7, 10, 11, 12},
    {1, 2, 3, 7, 10, 11, 12},
    {1, 2, 3, 7, 10, 11, 12},
    {1, 2, 3, 7, 10, 11, 12}}; // default CC numbers 1, 2, 3, 7, 10, 11, 12 //TODO: should be configurable maybe save in EEPROM

// enable these features with caution! it is GLITCHY!!
bool pcEnabled = false;      // TODO: should be configurable maybe save in EEPROM
bool ccEnabled = false;      // TODO: should be configurable maybe save in EEPROM
bool realTimeEnabled = true; // TODO: should be configurable maybe save in EEPROM
bool clockEnabled = true;    // TODO: should be configurable maybe save in EEPROM
byte groove = 6;             // default groove set to 6 (6/6 50% in LSDJ) //TODO: should be configurable maybe save in EEPROM

// Clocking
uint8_t ticks = 0;
uint64_t lastTickTime = 0;
float interval = 0.00;
uint32_t bpm = 0;

// Gameboy Serial Communication
uint64_t lastReadGameboy = 0;
bool isCommandWaiting = false;
byte commandWaiting = 0x00;
byte experimentalCorrection = 0x00;

// track 0-PU1, 1-PU2, 2-WAV, 3-NOI
byte lastNote[4] = {0, 0, 0, 0};
volatile byte velocity = 127; // adjustable Global Velocity, affecting all channels
byte lastTrack = 0;           // Track with the shortest update interval

// control interface
byte switchControlState;
bool lastControlState = HIGH;
bool controlPressed = false;
uint64_t lastPrint = 0;

// debouncing
bool stopFlag = false;
uint64_t idleTime = 0;

void clockReset()
{
  ticks = 0;
  lastTickTime = 0;
  interval = 0.00;
  bpm = 0;
}

void sendMIDI(midiEventPacket_t message)
{
  MidiUSB.sendMIDI(message);

#ifndef USE_MIDI_h
  Serial1.write(message.byte1);
  Serial1.write(message.byte2);
  Serial1.write(message.byte3);
#endif

  flushSerial();
}

void noteOn(byte track, byte note)
{
  noteStop(track); // stop previous note consider each track is monophonics

#ifdef USE_MIDI_h
  MIDI.sendNoteOn(note, velocity, outputChannel[track]);
#endif

  sendMIDI({0x09, static_cast<uint8_t>(0x90 | (outputChannel[track] - 1)), note, velocity});

  lastNote[track] = note;
}

void noteStop(byte track)
{
  byte note = lastNote[track];
  if (note)
  {
#ifdef USE_MIDI_h
    MIDI.sendNoteOff(note, 0x00, outputChannel[track]);
#endif

    sendMIDI({0x08, static_cast<uint8_t>(0x80 | (outputChannel[track] - 1)), note, 0x00});

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

void generateClock()
{
  for (uint8_t tick = 0; tick < groove; tick++)
  {
#ifdef USE_MIDI_h
    MIDI.sendRealTime(midi::Clock);
#endif
    sendMIDI({0x0F, 0xF8, 0x00, 0x00});
    delayMicroseconds(interval);
  }
}

void handleTick()
{
  if (ticks++ % groove == 0)
  {
    uint64_t currentTime = micros();
    if (lastTickTime > 0) // skip on first tap
    {
      interval = (currentTime - lastTickTime) / (groove * 1000);
      bpm = (round(60000.00 / (interval * PPQN)) / 5) * 5; // round to nearest 5
    }
    lastTickTime = currentTime - byteDelay; // byte offset correction

    if (clockEnabled)
      generateClock(); // 1st tick
  }
  if (ticks == PPQN)
    ticks = 0;
}

void checkStop() { //avoiding stop glitch
  if (stopFlag && idleTime > 50) // at least 6 idle time to consider stop
  {
      if (realTimeEnabled)
        {
    #ifdef USE_MIDI_h
          MIDI.sendRealTime(midi::Stop);
    #endif

          sendMIDI({0x0F, 0xFC, 0x00, 0x00});
        }
        clockReset();
        noteStopAll();

    #ifdef DEBUG_MODE
        Serial.println("Stop!");
    #endif

    stopFlag = false; 
    idleTime = 0;
  }
}


/* Reference from Arduinoboy (https://github.com/trash80/Arduinoboy)
    by Timothy Lamb @trash80
    ------------------------------------
    X## - Sends a MIDI CC -
    By default in Arduinoboy the high nibble selects a CC#, and the low nibble sends a value 0-F to 0-127.
    This can be changed to allow just 1 midi CC with a range of 00-6F, or 7 CCs with scaled or unscaled values.

    CC Mode:
    default: 1 for all 4 tracks
    - 0: use 1 midi CC, with the range of 00-6F,
    - 1: uses 7 midi CCs with the range of 0-F (the command's first digit would be the CC#)
    either way the value is scaled to 0-127 on output
    ------------------------------------
    CC Scaling:
    default: 1 for all 4 tracks
    - 1: scales the CC value range to 0-127 as oppose to lsdj's incomming 00-6F (0-112) or 0-F (0-15)
    - 0: no scaling, the value is directly sent to midi out 0-112 or 0-15
    scaling in c:
    byte scaledValue = map(value, 0, 112, 0, 127); // 0-112 to 0-127
    byte scaledValue = map(value, 0, 15, 0, 127); // 0-112 to 0-127
    ------------------------------------
    CC Message Numbers
    default options: {1,2,3,7,10,11,12} for each track, these options are CC numbers for lsdj midi out
    If CC Mode is 1, all 7 ccs options are used per channel at the cost of a limited resolution of 0-F
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

#ifdef USE_MIDI_h
  MIDI.sendControlChange(ccNumber, ccValue, outputChannel[track]);
#endif

  sendMIDI({0x0B, static_cast<uint8_t>(0xB0 | (outputChannel[track] - 1)), ccNumber, ccValue});

#ifdef DEBUG_MODE
  Serial.print("CC Command - track:");
  Serial.print(track);
  Serial.print(" value:");
  Serial.println(value);
#endif
}

void sendProgramChange(byte track, byte value)
{
#ifdef USE_MIDI_h
  MIDI.sendProgramChange(value, outputChannel[track]);
#endif

  sendMIDI({0x0C, static_cast<uint8_t>(0xC0 | (outputChannel[track] - 1)), value, 0x00});

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
    if (value == 0x7F) // use the GUNSHOT!!! y-FF command!
    {
      handleTick();
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
  switch (command)
  {
  case 0x7D: //Start!
    if (realTimeEnabled)
    {
#ifdef USE_MIDI_h
      MIDI.sendRealTime(midi::Start);
#endif

      sendMIDI({0x0F, 0xFA, 0x00, 0x00});
      stopFlag = false;
    }
    clockReset();
#ifdef DEBUG_MODE
    Serial.println("Start");
#endif
    break;
  case 0x7E: //Stop!!
    if (realTimeEnabled) stopFlag = true;
    break;
  case 0x7F: //consider this idle
    if (stopFlag) idleTime++;
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

#ifdef ARDUINOBOY_READER
/* This one based on Arduinoboy version, it is more stable on higher tempo
// Arduinoboy (https://github.com/trash80/Arduinoboy)
// by Timothy Lamb @trash80
*/
byte readIncomingByte()
{
  byte incomingMidiByte;
#ifndef NON_BLOCKING_DELAY
  delayMicroseconds(byteDelay);
#endif
  PORTF &= ~(1 << PF7); // LOW CLOCK_PIN
#ifndef NON_BLOCKING_DELAY
  delayMicroseconds(byteDelay);
#endif
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

#else
byte readIncomingByte()
{
  byte receivedByte = 0;
  PORTF |= (1 << PF6); // making sure Gameboy Serial In is HIGH! explanation: docs/references/gb_link_serial_in.md
  for (uint8_t i = 0; i < 8; i++)
  {
    PORTF |= (1 << PF7); // Set HIGH
    delayMicroseconds(BIT_DELAY);

    receivedByte = (receivedByte << 1) + ((PINF & (1 << PINF5)) ? 1 : 0); // Read a bit, and shift it into the byte
    // if (i == 0 && receivedByte == 0) return 0x7F;

    PORTF &= ~(1 << PF7); // Set LOW
    delayMicroseconds(BIT_DELAY);
  }

#ifndef NON_BLOCKING_DELAY
  delayMicroseconds(byteDelay);
#endif

  return receivedByte &= 0x7F; // Set the MSB range value to 0-127
}
#endif

void readGameboy()
{
#ifdef NON_BLOCKING_DELAY
  uint64_t currentTime = micros();
  if (currentTime - lastReadGameboy < byteDelay)
    return;
#endif

  byte value = readIncomingByte();

  lastReadGameboy = micros();

  if (isCommandWaiting)
  {
    routeMessage(commandWaiting, value);
    idleTime = 0;
    stopFlag = false;
    isCommandWaiting = false;

    // also read this for hiccups correction
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
      Serial.print("Hiccups! ticks??:");
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
    if (value == 0)
    {
      //ignore this!!! when its idle always 0
    }
    else if (experimentalCorrection)
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
  MidiUSB.flush();

#ifdef USE_MIDI_h
  MIDI.read();
#else
  Serial1.flush(); //??? not sure if this is necessary
#endif
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
  if (currentTime - lastPrint < 1000)
    return;

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
  checkStop();
}