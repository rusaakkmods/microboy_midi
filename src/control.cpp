#include "control.h"
#include "midi_controller.h"

byte switchControlState;
bool lastControlState = HIGH;
bool controlPressed = false;

ISR(PCINT0_vect)
{
  bool pinState = PINB & (1 << PB5);
  if (lastControlState == HIGH && pinState == LOW)
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
  lastControlState = pinState;
}

void control_read()
{
  switchControlState = digitalRead(ROTARY_SW);
  if (switchControlState == LOW && !controlPressed)
  {
    //midiController.velocity = 0;
    asm volatile ("jmp 0");
    
    controlPressed = true;
  }

  if (switchControlState == HIGH)
  {
    controlPressed = false;
  }
}

void control_init()
{
  pinMode(ROTARY_CLK, INPUT_PULLUP);
  pinMode(ROTARY_DT, INPUT);
  pinMode(ROTARY_SW, INPUT_PULLUP);

  PCICR |= (1 << PCIE0);   // Enable pin change interrupt for PCIE0 (Port B)
  PCMSK0 |= (1 << PCINT5); // Enable pin change interrupt for PB5 (PCINT5)
}