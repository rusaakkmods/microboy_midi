rusaaKKMODS - MicroBOY MIDI
---------------------------
'SPECIAL THANKS TO:'
  - Johan Kotlinski (@johan_kotlinski) for his work on LSDJ.
  - Timothy Lamb (@trash80) for his work on Arduinoboy. It's a great reference for me.
  - Nikita Bogdan (@nikitabogdan) & Alfian (instagram: @alfian_nay93) for briliant PC ideas!
  - AND of course our kooky AI: GPT 4o (ChatGPT, Copilot). Why not!!

 The code is still in the early stage, but it’s working as expected. The Gameboy is sending MIDI messages to the microcontroller, and the microcontroller is routing the messages to the MIDI output. 
 The next step is to add more features to the microcontroller, such as saving the configuration to EEPROM, adding a rotary encoder to change the output channel, and adding a button to change the CC mode and scaling.

  Reading Gameboy Message LSDJ MI.OUT MODE
  ----------------------------------------
  When LSDJ in MI.OUT mode; it is actually the Gameboy being Slave!!.
  In order for LSDJ send data byte, we have to send clock signal bit by bit. 
  Each clock signal send to the Gameboy will also trigger sending 1 bit of data out from Serial Out (SO) line.
  Read: docs/gb_clock.md
  Downside: LSDJ doesn't send Sequencer BPM clock information!

  This project is using Arduino Pro Micro / Leonardo
  --------------------------------------------------
  - I'll add compatibility for other microcontroller later on
  - Serial1 is used for MIDI communication
  - Serial is used for debugging purpose. disable it when not needed
  - CLOCK_PIN is used for Gameboy Clock -- Connect to Gameboy Serial Clock (SC) // it is full duplex may crash the Gameboy/LSDJ
  - SI_PIN is used for Message In (SI) -- Connect to Gameboy Serial Out (SO)
  - SO_PIN is used for Message Out (SO) -- Connect to Gameboy Serial In (SI)
  - By default, LSDJ use channel 1, 2, 3, 4 for each track respectively (PU1, PU2, WAV, NOI)
  
  Microboy Features
  -----------------
  - Sync with LSDJ MI.OUT Mode
  - Utilizing Program Change 127 (Gunshot) | LSDJ Y command FF to trigger Clock!
  - Adjustable Velocity knob
  - BPM calculation
  - Redirect each track to different MIDI channel
  - OLED Display information
  - Rotary Knob Control Setup