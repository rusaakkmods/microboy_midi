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
bool buttonPressed = false;
bool clicked = false;
bool shiftPressed = false;
bool comboPressed = false;

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
        Cursor cur = display.mainCursors[display.mainCursorIndex];
        switch (cur.trigger)
        {
          case SOLO:
            if (midiController.isSolo && midiController.soloTrack == display.mainCursorIndex + 1)
            {
              midiController.isSolo = false;
            }
            else
            {
              midiController.isSolo = true;
              midiController.soloTrack = display.mainCursorIndex + 1;
            }
            break;
          case MUTE:
            midiController.isMute = !midiController.isMute;
            break;
          default:
            //ignore for now
            break;
        }
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
      } else if (display.currentState == SUBMENU)
      {
        SubMenu sub = display.mainMenus[display.menuIndex].subMenus[display.submenuIndex];
        switch (sub.type)
        {
          case SAVE_CONFIG:
            config_save();
            display.currentState = MAIN_DISPLAY;
            break;
          case LOAD_DEFAULT:
            config_default();
            display.currentState = MAIN_DISPLAY;
            break;
          default:
            break;
        }
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
      map(delta, -1, 1, -1, 1);
      lastEncoderPosition = encoderPosition;

      switch (display.currentState)
      {
        case MAIN_DISPLAY:
          if (shiftPressed)
          {
            Cursor cur = display.mainCursors[display.mainCursorIndex];
            switch (cur.type)
            {
              case RANGE_1_16:
                config.outputChannel[display.mainCursorIndex] = constrain(config.outputChannel[display.mainCursorIndex] + delta, 1, 16);
                break;
              case RANGE_0_127:
                config.velocity = constrain(config.velocity + delta, 0, 127);
                break;
              default:
                //ignore for now
                break;
            }
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
            SubMenu sub = display.mainMenus[display.menuIndex].subMenus[display.submenuIndex];
            uint16_t value;
            switch (sub.type)
            {
              case ON_OFF:
                value = *sub.value == 0 ? 1 : 0;
                break;
              case RANGE_1_16:
                value = constrain(*sub.value + delta, 1, 16);
                break;
              case RANGE_0_127:
                value = constrain(*sub.value + delta, 0, 127);
                break;
              case RANGE_1000_5000_BY_100:
                value = constrain(*sub.value + delta * 100, 1000, 5000);
                break;
              default: //ignore for now
                break;
            }
            if (value != *sub.value)
            {
              *sub.value = value;
              display.updateFlag = true;  
            }
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
  if (midiController.isPU1Muted != digitalRead(MUTE_PU1))
    midiController.isPU1Muted = digitalRead(MUTE_PU1);
  if (midiController.isPU2Muted != digitalRead(MUTE_PU2))
    midiController.isPU2Muted = digitalRead(MUTE_PU2);
  if (midiController.isWAVMuted != digitalRead(MUTE_WAV))
    midiController.isWAVMuted = digitalRead(MUTE_WAV);
  if (midiController.isNOIMuted != digitalRead(MUTE_NOI))
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