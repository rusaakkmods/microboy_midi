#include "control.h"
#include "display.h"
#include "midi_controller.h"
#include <avr/interrupt.h>

// Rotary encoder variables
volatile int encoderPosition = 0;
volatile int lastEncoderPosition = 0; // Track the last encoder position to ensure precise steps
volatile int encoderState = 0;
volatile int lastEncoderState = 0;

// Button flags
volatile bool buttonPressed = false;
bool clicked = false;
volatile bool shiftPressed = false;
volatile bool comboPressed = false;
volatile bool adjustValueMode = false;



void control_checkNavigator()
{
  bool shiftState = digitalRead(SHIFT_PIN) == LOW;
  bool buttonState = digitalRead(BUTTON_PIN) == LOW;

  shiftPressed = shiftState;
  buttonPressed = buttonState;

  if (shiftPressed && buttonPressed)
  {
    if (!comboPressed)
    {
      comboPressed = true;
      if (display.currentState == MAIN_DISPLAY)
      {
        // todo control_trigger_on_off();
      }
      else if (display.currentState == MAIN_MENU)
      {
        display.currentState = MAIN_DISPLAY;
      }
      else if (display.currentState == SUBMENU)
      {
        display.currentState = MAIN_MENU;
      }
    }
  }
  else
  {
    comboPressed = false;
  }

  if (!shiftPressed || !buttonPressed)
  {
    if (!clicked && buttonPressed)
    {
      clicked = true;
      if (display.currentState == MAIN_DISPLAY)
      {
        display.currentState = MAIN_MENU;
      }
      else if (display.currentState == MAIN_MENU)
      {
        display.currentState = SUBMENU;
      }
    }
    else if (!buttonPressed)
    {
      clicked = false;
    }
  }
}

ISR(PCINT0_vect)
{
  static unsigned long lastInterruptTime = 0; // For debouncing
  static int lastEncoded = 0;                 // To track previous state for smoother transitions
  static int noiseFilterCounter = 0;          // Filter for noise

  unsigned long interruptTime = millis();

  if (interruptTime - lastInterruptTime > 5)
  {                                       // Adjust debounce delay to 5ms
    int MSB = digitalRead(ENCODER_PIN_A); // Most significant bit
    int LSB = digitalRead(ENCODER_PIN_B); // Least significant bit

    int encoded = (MSB << 1) | LSB;         // Combine the two pin states
    int sum = (lastEncoded << 2) | encoded; // Create a unique value for transition

    switch (sum)
    {
    case 0b0001:
    case 0b0111:
    case 0b1110:
    case 0b1000:
      noiseFilterCounter++;
      if (noiseFilterCounter >= 2)
      { // Require multiple valid signals to register
        encoderPosition++;
        noiseFilterCounter = 0;
      }
      break;
    case 0b0010:
    case 0b1011:
    case 0b1101:
    case 0b0100:
      noiseFilterCounter++;
      if (noiseFilterCounter >= 2)
      { // Require multiple valid signals to register
        encoderPosition--;
        noiseFilterCounter = 0;
      }
      break;
    default:
      // Invalid or noisy transition, reset filter
      noiseFilterCounter = 0;
      break;
    }

    lastEncoded = encoded;
    lastInterruptTime = interruptTime;

    if (encoderPosition != lastEncoderPosition)
    {
      int delta = encoderPosition - lastEncoderPosition;
      lastEncoderPosition = encoderPosition;

      switch (display.currentState)
      {
        case MAIN_DISPLAY:
          if (shiftPressed)
          {
            //todo update value
            //mainCursors[mainIndex].value += delta;
          }
          else
          {
            display.mainCursorIndex = constrain(display.mainCursorIndex + delta, 0, display.cursorSize - 1);
          }
          break;
        case MAIN_MENU:
          display.menuIndex = constrain(display.menuIndex + delta, 0, display.mainMenuSize - 1);
          break;
        case SUBMENU:
          if (shiftPressed)
          {
            //todo update value
            //mainMenu[menuIndex].subMenu[submenuIndex].value = constrain(mainMenu[menuIndex].subMenu[submenuIndex].value + delta, 0, 100);
          }
          else
          {
            display.submenuIndex = constrain(display.submenuIndex + delta, 0, display.mainMenus[display.menuIndex].size - 1);
          }
          break;
      }
    }
  }
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
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(SHIFT_PIN, INPUT_PULLUP);
  pinMode(ENCODER_PIN_A, INPUT_PULLUP);
  pinMode(ENCODER_PIN_B, INPUT_PULLUP);

  PCICR |= (1 << PCIE0);                   // Enable PCINT
  PCMSK0 |= (1 << PCINT5) | (1 << PCINT4); // Enable interrupts on pins 9 and 8

  sei();

  // // mute toggle switch
  pinMode(MUTE_PU1, INPUT_PULLUP);
  pinMode(MUTE_PU2, INPUT_PULLUP);
  pinMode(MUTE_WAV, INPUT_PULLUP);
  pinMode(MUTE_NOI, INPUT_PULLUP);
}