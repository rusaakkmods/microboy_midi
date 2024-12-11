#include "control.h"
#include "display.h"
#include "midi_controller.h"


bool lastRotaryState = HIGH;
byte enterState;
bool enterPressed = false;

bool shiftPressed() {
  return !digitalRead(BUTTON_SHIFT);
}

void control_checkNavigator()
{ 
  // read enter press
  enterState = digitalRead(ROTARY_SW);
  if (enterState == LOW && !enterPressed)
  {
    if (shiftPressed()) midiController.velocity = 0;
    else midiController.velocity = 100;
    //asm volatile ("jmp 0");
    enterPressed = true;
  }

  if (enterState == HIGH)
  {
    enterPressed = false;
  }
}

ISR(PCINT0_vect)
{
  bool pinState = PINB & (1 << PB5);
  if (lastRotaryState == HIGH && pinState == LOW)
  {
    if (digitalRead(ROTARY_DT))
    {
      midiController.velocity++;
    }
    else
    {
      midiController.velocity--;
    }
    midiController.velocity = constrain(midiController.velocity, 0, 127);
  }
  lastRotaryState = pinState;
}

void control_read()
{
  control_checkNavigator();

  // // mute toggle switch
  midiController.isPU1Muted = digitalRead(MUTE_PU1);
  midiController.isPU2Muted = digitalRead(MUTE_PU2);
  midiController.isWAVMuted = digitalRead(MUTE_WAV);
  midiController.isNOIMuted = digitalRead(MUTE_NOI);
}

void control_init()
{
  delay(1000); // wait for the OLED to initialize
  
  // navigator
  pinMode(ROTARY_CLK, INPUT_PULLUP);
  pinMode(ROTARY_DT, INPUT_PULLUP);

  PCICR |= (1 << PCIE0);   // Enable pin change interrupt for PCIE0 (Port B) 
  PCMSK0 |= (1 << PCINT5); // Enable pin change interrupt for PB5 (PCINT5)

  sei();

  // enter press
  pinMode(ROTARY_SW, INPUT_PULLUP);

  // //shift button
  pinMode(BUTTON_SHIFT, INPUT_PULLUP);

  // // mute toggle switch
  pinMode(MUTE_PU1, INPUT_PULLUP);
  pinMode(MUTE_PU2, INPUT_PULLUP);
  pinMode(MUTE_WAV, INPUT_PULLUP);
  pinMode(MUTE_NOI, INPUT_PULLUP);
}