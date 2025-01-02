#include "display.h"
#include "midi_controller.h"
#include "reader.h"
#include "control.h"

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

#ifdef DEBUG_MODE
  Serial.begin(9600); // to serial monitor
#endif

  config_init();
  reader_init();
  control_init();
  midi_init();
  display_init();
}

void loop()
{
    midi_handleStop(); // check stop flag
    control_read();
    reader_read();

    display_refresh();
}