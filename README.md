# midiGPIO
An interface between hardware inputs and outputs (voltage I/O or relay outputs) and MIDI.
It supports USB and serial MIDI and provides four 3.3 V voltage inputs and four 3.3 V voltage outputs.
Based on a Teensy 4.0 Plattform

# what can it be used for?
For example, for musical or artistic interaction between music and sensors or actuators.
To trigger cues in response to movement (door contact) or for synchronised triggering of an effect on stage when a note is played. 
Or as a multifunctional interface between a PC and guitar effects pedals (boards).
..... or for many other creative applications.

# how it works:
When a rising edge (>3 volts) is detected at the input, a MIDI Note-on command is sent.
By detecting a falling edge, a MIDI Note-off command is sent.
Alternatively, a MIDI program change can be sent when the edge rises.
The selection between the two modes is made using the mode switch.

To activate the relay outputs a MIDI Note-on command can be sent.
The Output (or Relay) remains active until a MIDI note-off command is received.

The following values are defined:
MIDI notes 60, 61, 62, 63 // (C3, C#3, D3, D#3)
at velocity >110. Applies to inputs and outputs.

In program change mode the following values are defined:
Program change no. = 1, 2, 3, 4

# board connections:
Refer to the Teensy 4.0 PinOut here for further details:
https://www.pjrc.com/store/teensy40.html

used pins:
1 = Serial MIDI rx
2 = Serial MIDI tx
3 = Input 1 (use with external pulldown resistror)
4 = Input 2 (use with external pulldown resistror)
5 = Input 3 (use with external pulldown resistror)
6 = Input 4 (use with external pulldown resistror)
12 = Midi Mode Switch (Teensy internal Pullup used)
13 = Output Status LED (Teensy onbard LED)
14 = Output 1
15 = Output 2
16 = Output 3
17 = Output 4

