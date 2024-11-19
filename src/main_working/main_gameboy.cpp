/*
  | WARNING: 
  | This knowledge is based on my own analysis of the Arduinoboy code and MIDI protocol. 
  | Follow at your own risk.

  1. Reading Gameboy Message LSDJ MI.OUT MODE
  ----------------------------------------
  When LSDJ in MI.OUT mode, it is actually the Gameboy being Slave.
  In order for LSDJ send data byte, we have to send clock signal bit by bit. 
  Each clock signal send to the Gameboy will also trigger sending 1 bit of data out from Serial Out (SO) line.
  Read: docs/gb_clock.md

  The data is sent in the following steps:
    1. Send a clock tick to the Gameboy
    2. Read the first bit of data, if it's high, meaning Gameboy will send the rest of 7 bit of the Byte next.
    3. The first bit is not data but a flag to continue reading next 7 bits as data.
    4. Continue by sending clock - reading bit out 7 times, to get the rest 7 bits of midi data one by one siftting left each time to the complete byte.
  This means that we have to send 8 clock ticks to the Gameboy to get 1 byte of data.

  The incoming byte received is not yet sent in MIDI message format type, instead it's ranged from 0-127 (7 bits of the byte where)
    MIDI Status Byte (1tttcccc)
      (MSB) of a status byte is always 1.
      ttt: Specifies the type of message (e.g., 100 for Note On, 101 for Control Change).
      cccc: Specifies the MIDI channel (0–15).
    MIDI Data Byte (0ddddddd) //todo: LSDJ send also 1 for MIDI Data Byte??
      (MSB) of a data byte is always 0.
      dddddd: Specifies the data value (0–127).
*/ 


#include <Arduino.h>


// Pin Definitions
#define SC_PIN 9    // Serial Clock (SC) from Game Boy (PB1, Pin 9)
#define SO_PIN 10   // Serial Out (SO) from Game Boy (PB2, Pin 10)

// Variables for Reading and Writing Data
volatile byte receivedByte = 0;   // Stores the received byte from SO
volatile int bitCount = 0;        // Tracks the number of bits read

void setup() {
  // Pin Modes
  pinMode(SC_PIN, INPUT);         // Set SC pin as input (Clock)
  pinMode(SO_PIN, INPUT);         // Set SO pin as input (Serial Out)

  // Initialize Serial Monitor for Debugging
  Serial.begin(9600);
}

void loop() {
  digitalWrite(SC_PIN, HIGH);
  if(digitalRead(SO_PIN)) {
    receivedByte = 0;
    for(int i = 0; i < 7; i++) {
      digitalWrite(SC_PIN, HIGH);
      receivedByte = (receivedByte << 1) + digitalRead(SO_PIN);
    }
    Serial.print("Received LSDJ Message: ");
    Serial.println(receivedByte, BIN);  // Print the received byte in binary
  }
}
