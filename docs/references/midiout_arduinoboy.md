Each of the 4 gameboy channels send MIDI data on 4 midi channels by the use of effects commands:

* `Nxx` - Sends a MIDI Note - Absolute to the value placed in the effect. N00 sends note off, `N01`-`N6F` send midi notes 1 to 112.
* `Qxx` - Sends a [MIDI Note](http://www.electronics.dit.ie/staff/tscarff/Music_technology/midi/midi_note_numbers_for_octaves.htm) relative to the current channel's pitch. The effect value is a offset. so `Q0C` in `PU1` would send a note 1 octave higher than what `PU1` is currently playing. This is useful as a table command to track midi notes as normal notes in the sequencer.
* `Xxx` - Sends a MIDI CC - By default in Arduinoboy the high nibble selects a CC#, and the low nibble sends a value `0-F` to `0-127`. This can be changed to allow just 1 midi CC with a range of `00`-`6F`, or 7 CCs with scaled or unscaled values.
* `Yxx` - Sends a program/patch/preset change.

By default each channel of LSDJ is mapped to midi channels 1-4. For example note commands from PU1 will be sent to midi channel 1. 