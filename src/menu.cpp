#include <U8g2lib.h>
#include <avr/interrupt.h>

// Pin definitions
#define ENCODER_PIN_A 8  // PCINT5
#define ENCODER_PIN_B 9  // PCINT4
#define BUTTON_PIN 7
#define BACK_PIN 6

// Rotary encoder variables
volatile int encoderPosition = 0;
volatile int lastEncoderPosition = 0; // Track the last encoder position to ensure precise steps
volatile int encoderState = 0;
volatile int lastEncoderState = 0;

// Button flags
volatile bool buttonPressed = false;
volatile bool backPressed = false;
volatile bool comboPressed = false;
volatile bool adjustValueMode = false;

// OLED setup
U8G2_SSD1306_128X32_UNIVISION_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);

// Menu state
enum MenuState { MAIN_DISPLAY, MAIN_MENU, SUBMENU };
MenuState currentState = MAIN_DISPLAY;

// Menu navigation variables
int currentMenuIndex = 0;   // Current menu or submenu index
int currentSubmenuIndex = 0;
int submenuValue[6] = {50, 75, 100, 20, 40, 60};  // Example values for submenus

void setup() {
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(BACK_PIN, INPUT_PULLUP);
  pinMode(ENCODER_PIN_A, INPUT_PULLUP);
  pinMode(ENCODER_PIN_B, INPUT_PULLUP);

  u8g2.begin();

  // Configure PCINT for encoder
  PCICR |= (1 << PCIE0); // Enable PCINT for pins 8-13
  PCMSK0 |= (1 << PCINT5) | (1 << PCINT4); // Enable interrupts on pins 9 and 8

  sei(); // Enable global interrupts
}

void loop() {
  // Handle button presses
  if (digitalRead(BUTTON_PIN) == LOW && digitalRead(BACK_PIN) == LOW && !comboPressed) {
    comboPressed = true;
    if (currentState == SUBMENU) {
      currentState = MAIN_MENU;
    }
  } else if (digitalRead(BUTTON_PIN) == HIGH || digitalRead(BACK_PIN) == HIGH) {
    comboPressed = false;
  }

  if (digitalRead(BUTTON_PIN) == LOW && !buttonPressed) {
    buttonPressed = true;
    if (currentState == MAIN_DISPLAY) {
      currentState = MAIN_MENU;
    } else if (currentState == MAIN_MENU) {
      currentState = SUBMENU;
      currentSubmenuIndex = 0;
    }
  } else if (digitalRead(BUTTON_PIN) == HIGH && buttonPressed) {
    buttonPressed = false;
  }

  if (digitalRead(BACK_PIN) == LOW && !backPressed) {
    backPressed = true;
    if (currentState == MAIN_MENU) {
      currentState = MAIN_DISPLAY; // Return to Main Display
    } else if (currentState == SUBMENU && !adjustValueMode) {
      currentState = MAIN_MENU; // Return to Menu Navigation Mode
    }
  } else if (digitalRead(BACK_PIN) == HIGH && backPressed) {
    backPressed = false;
  }

  switch (currentState) {
    case MAIN_DISPLAY:
      displayMainScreen();
      break;

    case MAIN_MENU:
      navigateMenu();
      break;

    case SUBMENU:
      navigateSubmenu();
      break;
  }
}

// --- Interrupt Service Routines ---
ISR(PCINT0_vect) {
  // Read the encoder pins
  int MSB = digitalRead(ENCODER_PIN_A); // Most significant bit
  int LSB = digitalRead(ENCODER_PIN_B); // Least significant bit

  int encoded = (MSB << 1) | LSB; // Combine the two pin states
  if (encoded != lastEncoderState) {
    if ((lastEncoderState == 0b00 && encoded == 0b01) ||
        (lastEncoderState == 0b01 && encoded == 0b11) ||
        (lastEncoderState == 0b11 && encoded == 0b10) ||
        (lastEncoderState == 0b10 && encoded == 0b00)) {
      encoderPosition++;
    } else if ((lastEncoderState == 0b00 && encoded == 0b10) ||
               (lastEncoderState == 0b10 && encoded == 0b11) ||
               (lastEncoderState == 0b11 && encoded == 0b01) ||
               (lastEncoderState == 0b01 && encoded == 0b00)) {
      encoderPosition--;
    }
    lastEncoderState = encoded;

    // Handle precise navigation
    if (encoderPosition != lastEncoderPosition) {
      if (currentState == MAIN_MENU) {
        if (encoderPosition > lastEncoderPosition) {
          currentMenuIndex = (currentMenuIndex + 1) % 3; // Loop within range
        } else {
          currentMenuIndex = (currentMenuIndex - 1 + 3) % 3; // Loop within range
        }
      } else if (currentState == SUBMENU) {
        if (digitalRead(BUTTON_PIN) == LOW) { // If button is pressed
          if (encoderPosition > lastEncoderPosition) {
            submenuValue[currentMenuIndex * 3 + currentSubmenuIndex] = constrain(submenuValue[currentMenuIndex * 3 + currentSubmenuIndex] + 1, 0, 100);
          } else {
            submenuValue[currentMenuIndex * 3 + currentSubmenuIndex] = constrain(submenuValue[currentMenuIndex * 3 + currentSubmenuIndex] - 1, 0, 100);
          }
        } else {
          if (encoderPosition > lastEncoderPosition) {
            currentSubmenuIndex = (currentSubmenuIndex + 1) % 3; // Loop within range
          } else {
            currentSubmenuIndex = (currentSubmenuIndex - 1 + 3) % 3; // Loop within range
          }
        }
      }
      lastEncoderPosition = encoderPosition;
    }
  }
}

// --- Display Functions ---
void displayMainScreen() {
u8g2.clearBuffer();
u8g2.setFontMode(1);
u8g2.setBitmapMode(1);
u8g2.setFont(u8g2_font_profont10_tr);
u8g2.drawStr(8, 8, "PU1");
u8g2.drawStr(40, 8, "PU2");
u8g2.setFont(u8g2_font_profont17_tr);
u8g2.drawStr(-2, 24, "[01]");
u8g2.drawStr(62, 24, "[02]");
u8g2.setFont(u8g2_font_profont10_tr);
u8g2.drawStr(72, 8, "WAV");
u8g2.setFont(u8g2_font_profont17_tr);
u8g2.drawStr(30, 24, "[10]");
u8g2.setFont(u8g2_font_profont10_tr);
u8g2.drawStr(104, 8, "NOI");
u8g2.setFont(u8g2_font_profont17_tr);
u8g2.drawStr(94, 24, "[--]");
u8g2.drawBox(34, 2, 2, 6);
u8g2.drawBox(2, 2, 2, 6);
u8g2.drawBox(98, 2, 2, 6);
u8g2.drawBox(66, 2, 2, 6);
u8g2.setDrawColor(2);
u8g2.drawBox(33, 1, 29, 8);
u8g2.drawBox(1, 1, 29, 8);
u8g2.drawBox(65, 1, 29, 8);
u8g2.drawBox(97, 1, 29, 8);
u8g2.sendBuffer();
}

void navigateMenu() {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_ncenB08_tr);
  u8g2.setCursor(0, 12);
  u8g2.print("Menu Item ");
  u8g2.print(currentMenuIndex + 1);
  u8g2.sendBuffer();
}

void navigateSubmenu() {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_ncenB08_tr);
  u8g2.setCursor(0, 12);
  u8g2.print("Submenu ");
  char indexLabel = 'A' + currentSubmenuIndex;
  u8g2.print(currentMenuIndex + 1);
  u8g2.print('.');
  u8g2.print(indexLabel);
  u8g2.print(": ");
  u8g2.print(submenuValue[currentMenuIndex * 3 + currentSubmenuIndex]);
  u8g2.sendBuffer();
}
