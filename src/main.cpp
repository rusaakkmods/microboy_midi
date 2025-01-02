#include "display.h"
#include "midi_controller.h"
#include "reader.h"
#include "control.h"

/* 
// TODO LIST:
// - Intead of Y-FF use Unused CC message for clocking??
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