


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
