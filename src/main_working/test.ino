// Arduino Pro Micro code to simulate a master clock that can be received by a Game Boy acting as a slave
#include <MIDI.h>

MIDI_CREATE_INSTANCE(HardwareSerial, Serial1, MIDI);

#define CLOCK_PIN A0      // Pin to generate the clock signal
#define SI_PIN A2        // Serial In (SI) pin to receive data from the Game Boy
#define SO_PIN A3
#define SC_PERIOD 125    // Clock period in microseconds (approx. 8 kHz)

volatile byte receivedByte = 0; // Stores the received byte
volatile int bitCount = 0;      // Track the number of bits received

volatile int tickCount = 0;

void setup() {
  pinMode(CLOCK_PIN, OUTPUT);    // Set clock pin as output
  pinMode(SI_PIN, INPUT); // Set SI pin as input with pull-up resistor
  pinMode(SO_PIN, INPUT); // Set SO pin as input with pull-up resistor for Game Boy Serial Out // Set SO pin as input with pull-up resistor for Game Boy Serial Out // Set SI pin as input with pull-up resistor
  Serial.begin(9600);            // Initialize Serial Monitor for debugging
  Serial1.begin(31250);
  //MIDI.begin(MIDI_CHANNEL_OMNI);
  
  Serial.println("Master clock simulation ready");
}

void loop() {
  // Generate the clock signal and read data from the Game Boy
  for (int i = 0; i < 8; i++) {
    digitalWrite(CLOCK_PIN, HIGH);   // Set clock HIGH
    delayMicroseconds(SC_PERIOD / 2); // Wait for half the clock period

    // Read data from Game Boy's SI pin
    bool bitValue = digitalRead(SI_PIN);
    if (i == 0 && bitValue != 1) break;
    receivedByte = (receivedByte << 1) + bitValue;

    digitalWrite(CLOCK_PIN, LOW);    // Set clock LOW
    delayMicroseconds(SC_PERIOD / 2); // Wait for the second half of the clock period

    bitCount++;
  }

  // If 8 bits are received, check for valid message byte and print the received byte
  if (bitCount >= 8) {
    receivedByte &= 0x7F; // Set the MSB to 0
    switch (receivedByte) {
        case 127:
          if (!tickCount) {
            MIDI.sendRealTime(midi::Clock);
          }
          tickCount++;
          if (tickCount == 24) {
            tickCount = 0;
          }
          break;
        case 126:
          MIDI.sendRealTime(midi::Stop);
          break;
        case 125:
          MIDI.sendRealTime(midi::Start);
          break;
    }
    if (receivedByte != 127) {
      Serial.print("Received Byte: ");
      Serial.println(receivedByte);
    }
    receivedByte = 0;
    bitCount = 0;
  }

  delay(10); // Small delay to allow Game Boy to prepare for the next byte
}


