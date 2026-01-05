/* midiGPIO (General Purpose Hardware In and Outputs controlled by Midi)
   Created by Mike Utz
  
   Works with USB and Serial Midi
   Contains 4 Inputs sending Midi Note or Programm Change Messages.
   One Input for chnage the Mode sending Midi Notes or Programm Change
   4 Outputs setting its state high or low by sending Midi Note On and Off.
   
   You must select Board: Teensy-LC or Tennsy-4.0 // USB-Type: Serial + MIDI from the "Tools menu.
  
   History:
   Version 0.1.  / First Edition)
   Version 0.2.  / Added dual MIDI output (USB + Serial) and input handling for both sources
   Version 0.2.1 / Changed Output to initiasl state HIGH for 4 Channel Relais Module (SeedStudio)
   Version 0.2.2 / Added Serial Output
*/

#include <MIDI.h>

// Created and binds the MIDI interface to the default hardware Serial port
MIDI_CREATE_INSTANCE(HardwareSerial, Serial1, MIDI);

// --- Configuration -------------------------------------------------------
const uint8_t inputPins[4]  = {2, 3, 4, 5};      // 4 physical input pins
const uint8_t outputPins[4] = {14, 15, 16, 17};  // 4 physical output pins
const uint8_t noteNumbers[4] = {60, 61, 62, 63}; // MIDI note numbers mapped to each IO

// Program change numbers to send in "program mode" (0..127)
const uint8_t programNumbers[4] = {0, 1, 2, 3};

const uint8_t MODE_PIN = 12;  // when HIGH => program-change mode; when LOW => note mode
const uint8_t STATE_PIN = 13; // shows the state of the Board = Ready

const uint8_t MIDI_CHANNEL = 1;   // 1..16 (set to desired channel). Use 1 for channel 1.
const unsigned long DEBOUNCE_MS = 10; // debounce interval in ms

// If true, a logical HIGH on an input means "active" => send Note On or Program Change.
// If false, a logical LOW means "active" (useful when wiring switches to GND + INPUT_PULLUP).
const bool INPUT_ACTIVE_HIGH = true;

// Velocity to send for Note On. Note Offs use velocity 0 (or send sendNoteOff).
const uint8_t NOTE_ON_VELOCITY = 110;

// -------------------------------------------------------------------------

// State tracking for inputs (debounce / change detection)
bool lastStableState[4];
bool lastReadState[4];
unsigned long lastDebounceTime[4];

// Forward declarations
void handleNoteOn(byte channel, byte note, byte velocity);
void handleNoteOff(byte channel, byte note, byte velocity);
void sendMidiNoteOn(uint8_t note, uint8_t velocity, uint8_t channel);
void sendMidiNoteOff(uint8_t note, uint8_t velocity, uint8_t channel);
void sendMidiProgramChange(uint8_t program, uint8_t channel);
void processNoteOn(byte channel, byte note, byte velocity);
void processNoteOff(byte channel, byte note, byte velocity);

void setup() {
  // Initialize Serial for debug if needed (uncomment to use).
  // Serial.begin(115200);
  // while (!Serial) { ; }

  // Configure input pins. Use INPUT_PULLUP if wiring switches to ground.
  for (uint8_t i = 0; i < 4; ++i) {
    // If you want external pull-downs or pushbutton to VCC, use INPUT
    // If buttons go to GND, use INPUT_PULLUP and set INPUT_ACTIVE_HIGH = false
    pinMode(inputPins[i], INPUT);
    lastReadState[i] = digitalRead(inputPins[i]);
    lastStableState[i] = lastReadState[i];
    lastDebounceTime[i] = millis();
  }

  // Configure MODE pin
  // If your switch pulls the pin to GND when active, use INPUT_PULLUP and invert logic accordingly.
  pinMode(MODE_PIN, INPUT_PULLUP);
  // If you prefer to use the internal pullup for MODE_PIN, change the line above to:
  // pinMode(MODE_PIN, INPUT_PULLUP);

  // Configure outputs
  for (uint8_t i = 0; i < 4; ++i) {
    pinMode(outputPins[i], OUTPUT);
    digitalWrite(outputPins[i], HIGH); // default OFF
  }

  // Configure STATE pin
  pinMode(STATE_PIN, OUTPUT);

  // Register MIDI handlers for USB MIDI
  usbMIDI.setHandleNoteOn(handleNoteOn);
  usbMIDI.setHandleNoteOff(handleNoteOff);

  // Register MIDI handlers for Serial MIDI
  MIDI.setHandleNoteOn(handleNoteOn);
  MIDI.setHandleNoteOff(handleNoteOff);

  // Start the Hardware MIDI Output and listen just to Channel 1
  MIDI.begin(MIDI_CHANNEL_OMNI);

  // Optionally send a startup message (comment out if not needed)
  // sendMidiNoteOn(60, 0, MIDI_CHANNEL);

  // Start Serial Output for print some messages
  Serial.begin(57600);

  // Set State Pin High
  digitalWrite(STATE_PIN, HIGH);
}

void loop() {
  // Process incoming USB and Serial MIDI (will call note handlers)
  while (MIDI.read()) {
    // will invoke our registered handlers.
  }
  while (usbMIDI.read()) {
    // usbMIDI.read() will invoke our registered handlers.
  }

  // Read current mode (HIGH => program-change mode)
  bool programMode = (digitalRead(MODE_PIN) == HIGH);

  // Poll inputs and send MIDI on change (with debounce)
  unsigned long now = millis();
  for (uint8_t i = 0; i < 4; ++i) {
    bool reading = digitalRead(inputPins[i]);

    // If the reading changed, reset the debounce timer
    if (reading != lastReadState[i]) {
      lastDebounceTime[i] = now;
      lastReadState[i] = reading;
    }

    // If the reading has been stable for longer than debounce interval,
    // consider it a valid state and check for a change from stable state
    if ((now - lastDebounceTime[i]) >= DEBOUNCE_MS) {
      if (lastStableState[i] != reading) {
        lastStableState[i] = reading;

        bool active;
        if (INPUT_ACTIVE_HIGH) {
          active = (reading == HIGH);
        } else {
          active = (reading == LOW);
        }

        uint8_t note = noteNumbers[i];
        if (active) {
          // Active: either send Program Change (in program mode) or Note On (in note mode)
          if (programMode) {
            uint8_t program = programNumbers[i];
            sendMidiProgramChange(program, MIDI_CHANNEL);
          } else {
            // Send Note On
            sendMidiNoteOn(note, NOTE_ON_VELOCITY, MIDI_CHANNEL);
          }
        } else {
          // In note mode, send Note Off on release. In program mode, do nothing on release.
          if (!programMode) {
            // Send Note Off
            sendMidiNoteOff(note, 0, MIDI_CHANNEL);
          } else {
            // program mode: no action on release (Program Change is a single event)
          }
        }
      }
    }
  }
  // Small idle delay — adjust as needed
  delay(1);
}

// -------------------------------------------------------------------------
// MIDI Output Functions (send to both USB and Serial MIDI)
// -------------------------------------------------------------------------

void sendMidiNoteOn(uint8_t note, uint8_t velocity, uint8_t channel) {
  // Send via USB MIDI
  usbMIDI.sendNoteOn(note, velocity, channel);
  // Send via Serial MIDI
  MIDI.sendNoteOn(note, velocity, channel);
  // Send Messaage to Serial Port
  Serial.println(String("Send noteOn: ch=") + channel + ", note=" + note + ", velocity=" + velocity);
}

void sendMidiNoteOff(uint8_t note, uint8_t velocity, uint8_t channel) {
  // Send via USB MIDI
  usbMIDI.sendNoteOff(note, velocity, channel);
  // Send via Serial MIDI
  MIDI.sendNoteOff(note, velocity, channel);
  // Send Messaage to Serial Port
  Serial.println(String("Send noteOff: ch=") + channel + ", note=" + note + ", velocity=" + velocity);
}

void sendMidiProgramChange(uint8_t program, uint8_t channel) {
  // Send via USB MIDI
  usbMIDI.sendProgramChange(program, channel);
  // Send via Serial MIDI
  MIDI.sendProgramChange(program, channel);
  // Send Messaage to Serial Port
  Serial.println(String("Send programChange") + program + ", channel" + channel);
}

// -------------------------------------------------------------------------
// MIDI Input Callbacks (unified handling for both USB and Serial MIDI)
// -------------------------------------------------------------------------

void handleNoteOn(byte channel, byte note, byte velocity) {
  // Process the note on message
  processNoteOn(channel, note, velocity);
  // Send Messaage to Serial Port
  Serial.println(String("Received noteOn: ch=") + channel + ", note=" + note + ", velocity=" + velocity);
}

void handleNoteOff(byte channel, byte note, byte velocity) {
  // Process the note off message
  processNoteOff(channel, note, velocity);
  // Send Messaage to Serial Port
  Serial.println(String("Received noteOff: ch=") + channel + ", note=" + note + ", velocity=" + velocity);  
}

// -------------------------------------------------------------------------
// MIDI Message Processing (handles output switching)
// -------------------------------------------------------------------------

void processNoteOn(byte channel, byte note, byte velocity) {
  // We can filter by channel if desired:
  if ((int)channel != MIDI_CHANNEL) {
    // comment out this return if you want to respond to all channels
    // return;
  }

  // If NoteOn with velocity 0, treat it as NoteOff (some senders do that)
  if (velocity == 0) {
    // treat as NoteOff
    processNoteOff(channel, note, velocity);
    return;
  }

  // Find matching note in our mapping and set the corresponding output HIGH
  for (uint8_t i = 0; i < 4; ++i) {
    if (noteNumbers[i] == note) {
      digitalWrite(outputPins[i], LOW);
      break;
    }
  }
}

void processNoteOff(byte channel, byte note, byte velocity) {
  if ((int)channel != MIDI_CHANNEL) {
    // comment out this return if you want to respond to all channels
    // return;
  }

  for (uint8_t i = 0; i < 4; ++i) {
    if (noteNumbers[i] == note) {
      digitalWrite(outputPins[i], HIGH);
      break;
    }
  }
}
