#include <MIDI.h>

MIDI_CREATE_INSTANCE(HardwareSerial, Serial1, MIDI);

#define CLOCK_PIN A0
#define SI_PIN A2
#define SO_PIN A3

//#define SC_PERIOD 125    // Gameboy Clock period in microseconds (approx. 8 kHz)
#define VELOCITY 127
#define CLOCK_DELAY 2
#define BYTE_DELAY 8

// track 0-PU1, 1-PU2, 2-WAV, 3-NOI
volatile byte lastNote[4] = {0, 0, 0, 0};
byte outputChannel[4] = {1, 2, 3, 4}; //default 1, 2, 3, 4

volatile long tickCount = 0;
volatile long lastTick = 0;

void setup() {
  pinMode(CLOCK_PIN, OUTPUT);
  pinMode(SI_PIN, INPUT);
  pinMode(SO_PIN, INPUT);

  digitalWrite(CLOCK_PIN, HIGH);

  Serial1.begin(31250);
  MIDI.begin(MIDI_CHANNEL_OMNI);
  
}

void playNote(byte track, byte note) {
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
  if (message < 0x04) {
    track = message;
    if (data) { // value > 0 then its a "Note On"
      stopNote(track); //stop previous note consider each track is monophonics
      playNote(track, data);
    }
    else {
      stopNote(track); // stop all note in channel
    }
  }
  else if (message < 0x08) {
    track = message - 4;
    //todo CC message
  }
  else if (message < 0x0C) {
    track = message - 8;
    //todo PC message
  }
  else {
    return; // Invalid message
  }
}

void sendMessage(byte message) {
  switch (message)
  {
    case 0x7F:
      //realtime clock message
      break;
    case 0x7E:
      MIDI.sendRealTime(midi::Stop);
      stopAll();
      break;
    case 0x7D:
      MIDI.sendRealTime(midi::Start);
      break;
    default:
      sendCommand(message - 0x70);
      break;
  }
}

byte readIncommingByte() {
  byte receivedByte = 0;
  
  for (int i = 0; i < 8; i++) {
    digitalWrite(CLOCK_PIN, HIGH); // Send HIGH Signal to Gameboy Clock
    delayMicroseconds(CLOCK_DELAY);

    receivedByte = (receivedByte << 1) + digitalRead(SI_PIN);

    digitalWrite(CLOCK_PIN, LOW);
    delayMicroseconds(CLOCK_DELAY);
  }

  delay(BYTE_DELAY); // Small delay to allow Game Boy to prepare for the next byte

  return receivedByte &= 0x7F; // Set the MSB range value to 0-127
}


void loop() {
  byte message = readIncommingByte();
  sendMessage(message);
}