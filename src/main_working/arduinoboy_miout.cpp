#include <Arduino.h>
#include <MIDI.h>
/**************************************************************************
 * Name:    Timothy Lamb                                                  *
 * Email:   trash80@gmail.com                                             *
 ***************************************************************************/
/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

//i'll use Pro Micro later for release version
MIDI_CREATE_INSTANCE(HardwareSerial, Serial1, MIDI);

#define GB_SET(bit_cl, bit_out, bit_in) PORTF = (PINF & B00011111) | ((bit_cl<<7) | ((bit_out)<<6) | ((bit_in)<<5))
// #define GB_READ_CL() ((PINF >> 7) & 0x01)
// #define GB_READ_SO() ((PINF >> 6) & 0x01)
// #define GB_READ_SI() ((PINF >> 5) & 0x01)

int SC_PIN = A0;    // Analog In 0 - clock out to gameboy
int SO_PIN = A1;    // Analog In 1 - serial data to gameboy
int SI_PIN = A2;    // Analog In 2 - serial data from gameboy

struct ValueMessage
{
    byte value1;
    byte value2;
};

ValueMessage message;
boolean readerValueMode;
int lastNote[4] = {-1,-1,-1,-1}; //per channel 1,2,3,4
byte incomingByte;
byte noteHoldCounter[4]; //per channel 1,2,3,4
byte noteHold[4][4]; //[channel][???]
unsigned long noteTimer[4]; // per channel
int noteTimerThreshold = 10;


// CC Mode, 0=use 1 midi CC, with the range of 00-6F (0-111), 
// 1 = uses 7 midi CCs with the range of 0-F (the command's first digit would be the CC#),
// either way the value is scaled to 0-127 on output
byte midiCCMode[4] = {1, 1, 1, 1}; // per channel 1,2,3,4 

// CC Scaling- Setting to 1 scales the CC value range to 0-127 as oppose to lsdj's incomming 00-6F (0-112) or 0-F (0-15)
byte midiCCScaling[4] = {1, 1, 1, 1};  // per channel 1,2,3,4 


//midiOutCCMessageNumbers - CC numbers for lsdj midi out, if CCMode is 1, all 7 ccs are used per channel at the cost of a limited resolution of 0-F
int midiCCNumber[28] = {
    1,2,3,7,10,11,12, //pu1
    1,2,3,7,10,11,12, //pu2
    1,2,3,7,10,11,12, //wav
    1,2,3,7,10,11,12, //noi
};


//int midioutBitDelay = 0; // gameboy clock rate approximately 8 KHz (8000 cycles per second), it is much slower than Arduino. it may need a delay

void sendClock() {
    GB_SET(0, 0, 0); //send low to clock via port. B000##### ( NOTE: # is unchanged any LSB in the port do not alter it may be used for other purposed)
    delayMicroseconds(125/2);
    GB_SET(1, 0, 0); //send low to clock via port. B100#####
    delayMicroseconds(125/2);
}

byte getIncommingByte()
{
    sendClock();
    if (digitalRead(SO_PIN)) //when 1st bit is high
    {
        //getting next 7 bit
        incomingByte = 0;
        for (int t = 0; t != 7; t++) //this make sure message between 0-127 message 
        {
            sendClock();
            incomingByte = (incomingByte << 1) + digitalRead(SO_PIN); //shift byte to left and update last bit
            delayMicroseconds(125/2);
        }
        delay(10);
        return true;
    }
    return false;
}

void playNote(byte channel, byte value)
{
    MIDI.sendNoteOn(value, 0x7F, channel+1); //fixed velocity 127
    Serial.print("Note On - ");
    Serial.print(value);
    Serial.print(", Channel: ");
    Serial.println(channel+1);

    noteHold[channel][noteHoldCounter[channel]] = value; //record note on hold percounter - per channel
    noteHoldCounter[channel]++; //record how long Note On
    noteTimer[channel] = millis(); //record last time Note On
    lastNote[channel] = value; //record last Note On
}

void stopNote(byte channel)
{
  for(int x = 0; x < noteHoldCounter[channel]; x++) { // until note on hold counter 
    byte noteNumber = noteHold[channel][x];
    MIDI.sendNoteOff(noteNumber, 0x00, channel+1); //TODO: not yet understand
    Serial.print("Note Off - ");
    Serial.print(noteNumber);
    Serial.print(", Channel: ");
    Serial.println(channel+1);
  }
  lastNote[channel] = -1; //reset last note in the channel
  noteHoldCounter[channel] = 0; //reset note hold counter
}

void checkStopNote(byte channel)
{
    if ((noteTimer[channel] + noteTimerThreshold) < millis())
    {
        stopNote(channel);
    }
}

void playCC(byte channel, byte value)
{
    byte controlValue = value;
    if (midiCCMode[channel])
    {
        if (midiCCScaling[channel])
        {
            controlValue = (value & 0x0F) * 8;
        }
        int ccNumberIndex = (channel * 7) + ((value >> 4) & 0x07); 
        int controlNumber = midiCCNumber[ccNumberIndex];

        MIDI.sendControlChange(controlNumber, controlValue, channel+1);
        Serial.print("CC - ");
        Serial.print(controlNumber);
        Serial.print(", Value: ");
        Serial.println(controlValue);
        Serial.print(", Channel: ");
        Serial.println(channel+1);
    }
    else
    {
        if (midiCCScaling[channel])
        {
            float s;
            s = value;
            controlValue = ((s / 0x6F) * 0x7F);
        }
        int ccNumberIndex = (channel * 7);
        int controlNumber = midiCCNumber[ccNumberIndex];

        MIDI.sendControlChange(controlNumber, controlValue, channel+1);
        Serial.print("CC - ");
        Serial.print(controlNumber);
        Serial.print(", Value: ");
        Serial.println(controlValue);
        Serial.print(", Channel: ");
        Serial.println(channel+1);
    }
}

void playPC(byte channel, byte value)
{
    MIDI.sendProgramChange(value, channel+1);
    Serial.print("PC - Value: ");
    Serial.println(value);
    Serial.print(", Channel: ");
    Serial.println(channel+1);
}

void sendMidiMessage(byte value1, byte value2)
{
    if (value1 < 0x04) // channel 0-3 is reffering to midi channel 1-4 , NOTE: fixed channel from LSDJ 0=PU1, 1=PU2, 2=WAV, 3=NOI
    {
        // And also by default when value1 is channel then instruction must be "Note On" or "Note Off"
        byte channel = value1;
        if (value2) // value > 0 then its a "Note On"
        {
            checkStopNote(channel); //check if theres note on floating
            playNote(channel, value2);
        }
        else if (lastNote[channel] >= 0) // value = 0 then its a "Note Off" and also checking just in case last Note in this channel is already off.
        {
            stopNote(channel); // stop all note in channel
        }
    }
    else if (value1 < 0x08) // value 4-7 this represent Control Change Message
    {
        byte channel = value1 - 4; //deduct the value by 4 to get the midi channel
        playCC(channel, value2);
    }
    else if (value1 < 0x0C) // value 8-11 this represent Control Change Message
    {
        byte channel = value1 - 8; //deduct the value by 8 to get the midi channel
        playPC(channel, value2);
    }
}

void stopAllNotes()
{
    for (int channel = 0; channel < 4; channel++) // Note off for every channel
    {
        if (lastNote[channel] >= 0) //get last note in the channel if exist then stop it
        {
            stopNote(channel); //send note off
        }
        // also send Control Change 123 - All Notes Off, set value 127, to the channel
        MIDI.sendControlChange(0x7B, 0x7F, channel+1);
        Serial.println("CC - All Notes Off, Channel:");
        Serial.println(channel+1);
    }
}

void setup()
{
    //DDRF |= (1 << PF7) | (1 << PF6) | (1 << PF5);
    pinMode(SC_PIN, OUTPUT);
    digitalWrite(SC_PIN, HIGH);


    Serial.begin(9600);
    Serial1.begin(31250);

    MIDI.begin(MIDI_CHANNEL_OMNI);

    message.value1 = -1;
    message.value2 = -1;
    readerValueMode = false;
}

void loop()
{
    
    if (getIncommingByte()) // when > 0 meaning theres a message
    {   //incoming byte could be 0 - 127
        if (incomingByte > 0x6F) // incomingMidiByte is 7 digits 112 - 127,
        {
            switch (incomingByte) //could be LSDJ Convention 125, 126, 127 considered as MIDI System realtime message (Start, Stop, Clock)
            {
                case 0x7F: // clock tick header number 127
                    //MIDI.sendRealTime(midi::Clock);
                    //Serial.println("Real Time - Clock");
                    break;
                case 0x7E: // seq stop header number 126
                    MIDI.sendRealTime(midi::Stop);
                    Serial.println("Real Time - Stop");
                    stopAllNotes(); //Make sure all notes are stopped
                    break;
                case 0x7D: // seq start header number 125
                    MIDI.sendRealTime(midi::Start);
                    Serial.println("Real Time -  Start");
                    break;
                default: //from 112 - 124 considered as Command which will later followed by 2 bytes of value
                    message.value1 = (incomingByte - 0x70); // MIDI Command Type assigned (-112 so instruction values within 0-15)
                    readerValueMode = true; //switch the reader flag so the next cycle will read the value if incomming byte < 112
                    break;
            }
        }
        else if (readerValueMode == true) // read the message value
        {
            Serial.println("TOTOTET");
            // by this time message value1 should range 0-15. incomming Byte could be 0-127
            sendMidiMessage(message.value1, incomingByte);
            readerValueMode = false; //switch back to false so the next cycle will evaluate the command
        }
    }
    
}



