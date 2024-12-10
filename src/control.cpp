#include "control.h"
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

static volatile uint32_t lastInterruptTime = 0;
static const uint16_t INTERRUPT_DEBOUNCE = 1; // ms

ISR(PCINT0_vect) {
    // Get current time for debounce
    uint32_t interruptTime = millis();
    if (interruptTime - lastInterruptTime < INTERRUPT_DEBOUNCE) {
        return;
    }

    // Read both pins in sequence
    bool clkState = PINB & (1 << PB5);
    bool dtState = digitalRead(ROTARY_DT);
    
    // Only trigger on valid falling edge of CLK
    if (lastRotaryState == HIGH && clkState == LOW) {
        // Valid transition detected
        if (dtState) {
            midiController.velocity++;
        } else {
            midiController.velocity--;
        }
        midiController.velocity = constrain(midiController.velocity, 0, 127);
        lastInterruptTime = interruptTime;
    }
    
    lastRotaryState = clkState;
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
  // navigator
  pinMode(ROTARY_CLK, INPUT_PULLUP);
  pinMode(ROTARY_DT, INPUT_PULLUP);

  //PCICR |= (1 << PCIE0);   // Enable pin change interrupt for PCIE0 (Port B) 
  //PCMSK0 |= (1 << PCINT5); // Enable pin change interrupt for PB5 (PCINT5)

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