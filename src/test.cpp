#include <U8g2lib.h>
#include <avr/interrupt.h>

// Pin definitions
#define ENCODER_PIN_A 8  // PCINT5
#define ENCODER_PIN_B 9  // PCINT4
#define BUTTON_PIN 7
#define SHIFT_PIN 6

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

// Menu state
enum MenuState { 
    MAIN_DISPLAY, 
    MAIN_MENU, 
    SUBMENU 
};
MenuState currentState = MAIN_DISPLAY;

// Menu navigation variables
int mainIndex = 0;
int menuIndex = 0;
int submenuIndex = 0;

// OLED setup
U8G2_SSD1306_128X32_UNIVISION_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);

struct Cursor {
  int x;
  int y;
  int w;
  int h;
  int value;
};

#define NUM_MAIN_CURSORS 5
#define NUM_MENU_CURSORS 3

Cursor cursors[NUM_MAIN_CURSORS] = {
    {1, 8, 29, 15, 0},
    {33, 8, 29, 15, 0},
    {65, 8, 29, 15, 0},
    {97, 8, 29, 15, 0},
    {112, 24, 15, 8, 0}
};

struct SubMenu {
    char *name;
    int value;
};

struct MenuItem {
    char *name;
    SubMenu* subMenu;
};

SubMenu tracks[4] = {
    {"PU1", 1},
    {"PU2", 2},
    {"WAV", 3},
    {"NOI", 4}
};

SubMenu global[4] = {
    {"Velocity", 100},
    {"BPM", 120},
    {"Groove", 50},
    {"Clock", 0}
};

MenuItem mainMenu[2] = {
    {"Global", global},
    {"Tracks", tracks}
};

void setup() {
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(SHIFT_PIN, INPUT_PULLUP);
  pinMode(ENCODER_PIN_A, INPUT_PULLUP);
  pinMode(ENCODER_PIN_B, INPUT_PULLUP);

  PCICR |= (1 << PCIE0); // Enable PCINT
  PCMSK0 |= (1 << PCINT5) | (1 << PCINT4); // Enable interrupts on pins 9 and 8

  sei();

  u8g2.begin();
}

void triggerMainOnOff() {
    if (currentState == MAIN_DISPLAY) {
        // TODO: Toggle solo/un-solo channel | mute/un-mute velocity
    }
}

void readStates() {
    if (!shiftPressed && digitalRead(SHIFT_PIN) == LOW) {
        shiftPressed = true;
    } else if (digitalRead(SHIFT_PIN) == HIGH) {
        shiftPressed = false;
    }

    if (!buttonPressed && digitalRead(BUTTON_PIN) == LOW) {
        buttonPressed = true;
    } else if (digitalRead(BUTTON_PIN) == HIGH) {
        buttonPressed = false;
        clicked = false;
    }

    if (!comboPressed && shiftPressed && buttonPressed) {
        comboPressed = true;
        if (currentState == MAIN_DISPLAY) {
            triggerMainOnOff();
        } else if (currentState == MAIN_MENU) {
            currentState = MAIN_DISPLAY;
        } else if (currentState == SUBMENU) {
            currentState = MAIN_MENU;
        }
    } 
    
    if (!shiftPressed || !buttonPressed) {
        comboPressed = false;
        if (!clicked && buttonPressed) {
            clicked = true;
            if (currentState == MAIN_DISPLAY) {
                currentState = MAIN_MENU;
            } else if (currentState == MAIN_MENU) {
                currentState = SUBMENU;
            }
        }
    }
}

void loop() {
    readStates();

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
  static unsigned long lastInterruptTime = 0; // For debouncing
  static int lastEncoded = 0; // To track previous state for smoother transitions
  static int noiseFilterCounter = 0; // Filter for noise

  unsigned long interruptTime = millis();

  if (interruptTime - lastInterruptTime > 5) { // Adjust debounce delay to 10ms
    int MSB = digitalRead(ENCODER_PIN_A); // Most significant bit
    int LSB = digitalRead(ENCODER_PIN_B); // Least significant bit

    int encoded = (MSB << 1) | LSB; // Combine the two pin states
    int sum = (lastEncoded << 2) | encoded; // Create a unique value for transition

    switch (sum) {
      case 0b0001:
      case 0b0111:
      case 0b1110:
      case 0b1000:
        noiseFilterCounter++;
        if (noiseFilterCounter >= 2) { // Require multiple valid signals to register
          encoderPosition++;
          noiseFilterCounter = 0;
        }
        break;
      case 0b0010:
      case 0b1011:
      case 0b1101:
      case 0b0100:
        noiseFilterCounter++;
        if (noiseFilterCounter >= 2) { // Require multiple valid signals to register
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

    if (encoderPosition != lastEncoderPosition) {
      if (currentState == MAIN_DISPLAY) {
        if (encoderPosition > lastEncoderPosition) {
          if (shiftPressed) {
            cursors[mainIndex].value += 1;
          } else {
            mainIndex = (mainIndex + 1) % NUM_MAIN_CURSORS;
          }
        } else {
          if (shiftPressed) {
            cursors[mainIndex].value -= 1;
          } else {
            mainIndex = (mainIndex - 1 + NUM_MAIN_CURSORS) % NUM_MAIN_CURSORS;
          }
        }
      } else if (currentState == MAIN_MENU) {
        if (encoderPosition > lastEncoderPosition) {
          menuIndex = (menuIndex + 1) % 2;
        } else {
          menuIndex = (menuIndex - 1 + 2) % 2;
        }
      } else if (currentState == SUBMENU) {
        if (encoderPosition > lastEncoderPosition) {
          if (shiftPressed) {
            mainMenu[menuIndex].subMenu[submenuIndex].value = constrain(mainMenu[menuIndex].subMenu[submenuIndex].value + 1, 0, 100);
          } else {
            submenuIndex = (submenuIndex + 1) % 4;
          }
        } else {
          if (shiftPressed) {
            mainMenu[menuIndex].subMenu[submenuIndex].value = constrain(mainMenu[menuIndex].subMenu[submenuIndex].value - 1, 0, 100);
          } else {
            submenuIndex = (submenuIndex - 1 + 4) % 4;
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
u8g2.drawBox(18, 1, 12, 6);
u8g2.setFont(u8g2_font_profont10_tr);
u8g2.drawStr(1, 7, "PU1");
u8g2.setFont(u8g2_font_profont17_tr);
u8g2.drawStr(-2, 21, "[01]");
u8g2.drawBox(50, 1, 12, 6);
u8g2.setFont(u8g2_font_profont10_tr);
u8g2.drawStr(34, 7, "PU2");
u8g2.setFont(u8g2_font_profont17_tr);
u8g2.drawStr(30, 21, "[10]");
u8g2.drawBox(82, 1, 12, 6);
u8g2.setFont(u8g2_font_profont10_tr);
u8g2.drawStr(65, 7, "WAV");
u8g2.setFont(u8g2_font_profont17_tr);
u8g2.drawStr(62, 21, "[02]");
u8g2.drawBox(114, 1, 12, 6);
u8g2.setDrawColor(2);
u8g2.drawBox(51, 4, 10, 2);
u8g2.drawBox(19, 4, 10, 2);
u8g2.setDrawColor(1);
u8g2.setFont(u8g2_font_profont10_tr);
u8g2.drawStr(97, 7, "NOI");
u8g2.setDrawColor(2);
u8g2.drawBox(83, 4, 8, 2);
u8g2.setDrawColor(1);
u8g2.setFont(u8g2_font_profont17_tr);
u8g2.drawStr(94, 21, "[--]");
u8g2.drawBox(1, 25, 110, 6);
u8g2.setDrawColor(2);
u8g2.setFont(u8g2_font_profont10_tr);
u8g2.drawStr(2, 30, "v");
u8g2.setDrawColor(1);
u8g2.drawStr(112, 31, "100");
u8g2.setDrawColor(2);
u8g2.drawBox(7, 26, 87, 4);
u8g2.drawStr(115, 6, "m");

Cursor cu = cursors[mainIndex];
u8g2.drawBox(cu.x, cu.y, cu.w, cu.h);

u8g2.sendBuffer();
}

void navigateMenu() {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_ncenB08_tr);
  u8g2.setCursor(0, 12);
  u8g2.print(mainMenu[menuIndex].name);
  u8g2.sendBuffer();
}

void navigateSubmenu() {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_ncenB08_tr);
  u8g2.setCursor(0, 12);
  u8g2.print(mainMenu[menuIndex].subMenu[submenuIndex].name);
  u8g2.print(": ");
  u8g2.print(mainMenu[menuIndex].subMenu[submenuIndex].value);
  u8g2.sendBuffer();
}
