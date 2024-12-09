#include "display.h"
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32
#define OLED_I2C_ADDRESS 0x3C

Display display;

Adafruit_SSD1306 oled(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
uint64_t lastPrint = 0;

/*
    MAIN MENU:
    1. Showing each channel output
    2. Showing Velocity and approximate BPM and Groove(tps) tick per step
    
    Next:
    -> PU1 select MIDI Channel
    -> PU2 select MIDI Channel
    -> WAV select MIDI Channel
    -> NOI select MIDI Channel

    Next:
    -> Enable/Disable Control Change
    -> Enable/Disable Program Change
    -> Enable/Disable Start/Stop (Realtime)
    -> Enable/Disable Clock
    
    Next:
    -> Set Groove (tick per step)
    
    Next:
    -> Set Velocity

    Next:
    -> 
*/

void display_splash()
{
    oled.clearDisplay();
    oled.setCursor(0, 0);
    oled.println(_USB_PRODUCT);
    oled.setCursor(0, 10);
    oled.println(VERSION);
    oled.setCursor(0, 20);
    oled.print(_USB_MANUFACTURER);
    oled.display();
}

void display_init()
{
    Wire.begin();
    oled.begin(SSD1306_SWITCHCAPVCC, OLED_I2C_ADDRESS);
    oled.setTextSize(1);
    oled.setTextColor(SSD1306_WHITE);

    display_splash();
}

void display_main()
{
    uint64_t currentTime = millis();
    if (currentTime - lastPrint < 1000)
        return;
    lastPrint = currentTime;

    oled.clearDisplay();
    oled.setCursor(0, 0);
    oled.print("PU1 [");
    oled.print(String(config.outputChannel[0] < 10 ? "0" : "") + String(config.outputChannel[0]));
    oled.print("] | PU2 [");
    oled.print(String(config.outputChannel[1] < 10 ? "0" : "") + String(config.outputChannel[1]));
    oled.println("]");
    oled.setCursor(0, 10);
    oled.print("WAV [");
    oled.print(String(config.outputChannel[2] < 10 ? "0" : "") + String(config.outputChannel[2]));
    oled.print("] | NOI [");
    oled.print(String(config.outputChannel[3] < 10 ? "0" : "") + String(config.outputChannel[3]));
    oled.println("]");
    oled.setCursor(0, 20);
    oled.print("VEL:");
    oled.print(display.velocity);
    oled.print(" BPM:");
    oled.print(display.bpm);
    oled.display();
}