#include "display.h"
#include "midi_controller.h"
#include "clock.h"
#include "reader.h"
#include "control.h"

// TODO LIST:
// - Implement OLED Menu for Configurations
// - Exclude unused CC message - this could minimize glitches
// - Intead of Y-FF use Unused CC message for clocking??
// - add reset function
// - add track leds
// - add track mute/unmute
// - add track solo
// - rename USB_PRODUCT to microboy

void setup()
{
  //config_init();
  config_default();
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

  // todo: values must be triggered by eventhandler
  display.bpm = clock.bpm;
  display.velocity = midiController.velocity;
  display_main();
}