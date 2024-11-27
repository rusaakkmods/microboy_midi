#pragma once

#define _USB_PRODUCT "MicroBOY MIDI"
#define _USB_MANUFACTURER "rusaaKKMODS"
#define VERSION "v0.0.1"
#define YEAR "2024"

 #define DEBUG_MODE /* enable debug mode */
// #define USE_MIDI_h               /* use MIDI Library for MIDI Serial Communication (faster but unstable! buffer overflow in faster rate) */
// #define ARDUINOBOY_READER        /* use Arduinoboy version of reader also stable, tweak byteDelay for better stability */
 #define NON_BLOCKING_DELAY /* use non-blocking read for Gameboy Serial Communication (more stable when enable clock) */