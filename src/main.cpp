#include "display.h"
#include "midi_controller.h"
#include "clock.h"
#include "reader.h"
#include "control.h"
//#include <avr/wdt.h>

/* 
// TODO LIST:
// - Implement OLED Menu for Configurations
// - Exclude unused CC message - this could minimize glitches
// - Intead of Y-FF use Unused CC message for clocking??
// - add reset function
// - add track leds
// - add track mute/unmute
// - add track solo
// - rename USB_PRODUCT to microboy
// - todo check if usb not connected to midi device using Serial.dtr()
*/

void setup()
{
  //wdt_enable(WDTO_1S); // Set watchdog to 2 seconds

#ifdef DEBUG_MODE
  Serial.begin(9600); // to serial monitor
#endif

  // config_init();
  config_default();
  reader_init();
  control_init();
  midi_init();
  display_init();
}

void loop()
{
  //wdt_reset(); // Reset watchdog disabled causing unable to upload 

  // reader_checkConnection(); // check connection

  // if (pinChecker.isConnected) {
    midi_handleStop(); // check stop flag
    control_read();
    reader_read();

    // todo: these values must be triggered by eventhandler
    display.bpm = clock.bpm;
    display.velocity = midiController.velocity;

    display_main();
  // } else {
  //   display_disconnected();
  // }
}