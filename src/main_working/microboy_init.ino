// Arduino Pro Micro code to simulate a master clock that can be received by a Game Boy acting as a slave
#include <MIDI.h>

MIDI_CREATE_INSTANCE(HardwareSerial, Serial1, MIDI);

#define GB_SET(bit_cl, bit_out, bit_in) PORTF = (PINF & B00011111) | ((bit_cl<<7) | ((bit_out)<<6) | ((bit_in)<<5))

#define CLOCK_PIN A0      // Pin to generate the clock signal
#define SI_PIN A2        // Serial In (SI) pin to receive data from the Game Boy
#define SO_PIN A3
#define SC_PERIOD 125    // Clock period in microseconds (approx. 8 kHz)

volatile byte receivedByte = 0; // Stores the received byte
volatile int bitCount = 0;      // Track the number of bits received
volatile boolean valueIn = false;

volatile unsigned long lastTick = 0;
volatile int tickCount = 0;

volatile byte command = 0;
volatile byte data = 0;

volatile byte lastNote[4] = {0, 0, 0, 0};

void setup() {
  pinMode(CLOCK_PIN, OUTPUT);    // Set clock pin as output
  pinMode(SI_PIN, INPUT); // Set SI pin as input with pull-up resistor
  pinMode(SO_PIN, INPUT); // Set SO pin as input with pull-up resistor for Game Boy Serial Out // Set SO pin as input with pull-up resistor for Game Boy Serial Out // Set SI pin as input with pull-up resistor

  digitalWrite(CLOCK_PIN, HIGH);

  //Serial.begin(9600);            // Initialize Serial Monitor for debugging
  Serial1.begin(31250);
  MIDI.begin(MIDI_CHANNEL_OMNI);
  
  //Serial.println("Master clock simulation ready");
}

void playNote(byte channel, byte data) {
    MIDI.sendNoteOn(data, 0x7F, channel+1);
    lastNote[channel] = data;
}

void stopNote(byte channel) {
  byte note = lastNote[channel];
  if (note) {
    MIDI.sendNoteOff(note, 0x00, channel+1);
    lastNote[channel] = 0;
  }
}

void stopAll() {
  for (int ch = 0; ch < 4; ch++) {
    stopNote(ch);
  }
}

void doMessage() {
  if (command < 0x04) // channel 0-3 is reffering to midi channel 1-4 , NOTE: fixed channel from LSDJ 0=PU1, 1=PU2, 2=WAV, 3=NOI
    {
        // And also by default when value1 is channel then instruction must be "Note On" or "Note Off"
        byte channel = command;
        if (data) // value > 0 then its a "Note On"
        {
            //stop previous note //consider gameboy is monophonics
            stopNote(channel);
            byte note = data;
            playNote(channel, note);
        }
        else
        {
            stopNote(channel); // stop all note in channel
        }
    }
    else if (command < 0x08) // value 4-7 this represent Control Change Message
    {
        byte channel = command - 4; //deduct the value by 4 to get the midi channel
        //playCC(channel, value2);
        
        // Serial.print("CC on Channel: ");
        // Serial.print(channel);
        // Serial.print(", value: ");
        // Serial.println(data);
    }
    else if (command < 0x0C) // value 8-11 this represent Control Change Message
    {
        byte channel = command - 8; //deduct the value by 8 to get the midi channel
        //playPC(channel, value2);

        // Serial.print("PC on Channel: ");
        // Serial.print(channel);
        // Serial.print(", value: ");
        // Serial.println(data);
    }
}

void loop() {
  // Generate the clock signal and read data from the Game Boy
  for (int i = 0; i < 8; i++) {
    digitalWrite(CLOCK_PIN, HIGH);   // Set clock HIGH
    //GB_SET(1,0,0);
    delayMicroseconds(2); // Wait for half the clock period

    // Read data from Game Boy's SI pin
    bool bitValue = digitalRead(SI_PIN);
    receivedByte = (receivedByte << 1) + bitValue;

    digitalWrite(CLOCK_PIN, LOW);    // Set clock LOW
    //GB_SET(0,0,0);
    delayMicroseconds(2); // Wait for the second half of the clock period

    bitCount++;
  }

  // If 8 bits are received, check for valid message byte and print the received byte
  if (bitCount >= 8) {
    receivedByte &= 0x7F; // Set the MSB to 0
    if (valueIn) {
      data = receivedByte;
      //Serial.println(data);
      doMessage();
      valueIn = false;
    }
    else {
      switch (receivedByte) {
          case 127:
            //MIDI.sendRealTime(midi::Clock);
            // tickCount++;
            // unsigned long currentTime = millis();
            // if (currentTime - lastTick >= 1000) {
            //   lastTick = currentTime;
            //   Serial.print("TPS: ");
            //   Serial.println(tickCount);
            //   tickCount = 0;
            // }
            break;
          case 126:
            MIDI.sendRealTime(midi::Stop);
            stopAll();
            break;
          case 125:
            MIDI.sendRealTime(midi::Start);
            break;
          default:
            command = receivedByte - 0x70;
            valueIn = true;
            break;
      }
    }
    receivedByte = 0;
    bitCount = 0;
  }
  //delayMicroseconds(125);
  delay(8); // Small delay to allow Game Boy to prepare for the next byte
}


