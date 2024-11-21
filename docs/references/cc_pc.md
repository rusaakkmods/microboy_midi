**MIDI Control Change (CC)**

In MIDI (Musical Instrument Digital Interface), **Control Change (CC)** is a type of message used to convey specific real-time changes in a musical performance. It is often used to modify parameters like volume, pan, modulation, effects, and many others, allowing a musician to have expressive control over the sound.

### **Details of Control Change (CC):**
- **Message Structure**: Control Change messages consist of three bytes:
  1. **Status Byte**: Indicates that the message is a Control Change, followed by the MIDI channel number (0–15).
  2. **Controller Number Byte**: Identifies which control is being affected. This number ranges from **0 to 127**, and each number represents a specific control, such as modulation wheel (CC 1), sustain pedal (CC 64), etc.
  3. **Controller Value Byte**: Represents the value for that controller, ranging from **0 to 127**. For example, with a volume control, a value of 0 would mean no sound, while 127 would mean full volume.

- **Typical Control Change Uses**:
  - **CC 1 (Modulation Wheel)**: Controls modulation, often used to add vibrato or other effects.
  - **CC 7 (Channel Volume)**: Controls the main volume of a MIDI channel.
  - **CC 10 (Pan)**: Adjusts the stereo position of the sound.
  - **CC 64 (Sustain Pedal)**: Acts as a sustain pedal on a piano—values 0 to 63 are "off", and 64 to 127 are "on".

- **Range**: The **controller number** (0–127) is mapped to various controls, and the **value** (0–127) represents the intensity or state of that control.

- **Real-Time Interaction**: Control Change messages are typically used for **real-time** interaction with the synthesizer or instrument. For example, moving a modulation wheel while playing would send a series of Control Change messages that modify the sound in real time.

- **MIDI Learn**: Many software instruments and DAWs support a feature called **MIDI Learn**, where you can map a physical MIDI controller knob or fader to any virtual parameter using a Control Change message.

- **Common CC Numbers**:
  - **CC 0**: Bank Select (MSB)
  - **CC 1**: Modulation Wheel
  - **CC 7**: Channel Volume
  - **CC 10**: Pan
  - **CC 11**: Expression Controller
  - **CC 64**: Sustain Pedal (Hold Pedal)
  - **CC 120–127**: Channel Mode Messages (like "All Sound Off" or "Reset All Controllers")

Control Change messages are crucial for making electronic music dynamic and expressive, as they allow a performer to tweak many aspects of a sound or performance in real time.

---

**MIDI Program Change (PC)**

A **Program Change** (often called **Patch Change**) is a type of MIDI message used to change the **instrument sound** or **preset** on a synthesizer, sound module, or any other MIDI-compatible device. This allows musicians to quickly switch between different sounds, such as piano, strings, guitar, or any other patches, during a performance.

### **Details of Program Change:**
- **Message Structure**: A Program Change message consists of two bytes:
  1. **Status Byte**: Indicates that the message is a Program Change and also includes the **MIDI channel** number (0-15).
  2. **Program Number Byte**: Identifies the **program (preset)** to be selected. This number ranges from **0 to 127**, representing the preset number on the synthesizer or device.

- **Use in Performances**: During live performances or recording sessions, a musician can send Program Change messages to seamlessly switch between different sounds without having to manually adjust settings on the hardware or software. For example, in a sequence, you might switch from a **piano sound (Program 0)** to a **guitar sound (Program 24)** with a Program Change message.

- **Program Numbers**: Program numbers vary from 0 to 127, and the specific mapping of numbers to sounds depends on the synthesizer or General MIDI specification. General MIDI defines a standard set of 128 instruments, such as:
  - **0**: Acoustic Grand Piano
  - **24**: Nylon Guitar
  - **40**: Violin
  - **73**: Flute
  - **127**: Gunshot

  However, in many modern synths or devices, the specific sound might not adhere to the General MIDI standard, and manufacturers often use custom sound banks.

- **Bank Select**: Some synthesizers or devices have more than 128 sounds. In these cases, **Bank Select** messages (Control Change **CC 0** and **CC 32**) are used in combination with Program Change messages to switch between different banks of programs. Essentially, the Bank Select defines which set of 128 sounds you are switching among, and the Program Change then selects the specific sound from that bank.

- **Example Use Case**:
  - If a keyboard player wants to switch from a **piano sound** to a **synth pad** while playing, they can send a Program Change message to the keyboard. This switch can be triggered via a MIDI controller, foot pedal, or even be embedded in a sequencer track.

### **Summary:**
- A **Program Change** message is a simple and powerful way to select different instrument presets or patches.
- It typically consists of a **status byte** and a **program number byte**.
- The value of the program number is **0-127**, selecting from the available presets in the device.
- It is often combined with **Bank Select** to access more than 128 sounds on advanced synthesizers.

Program Change messages are fundamental in MIDI for creating dynamic, expressive, and diverse musical performances and arrangements, allowing musicians to instantly switch between different timbres and sounds during a piece.

