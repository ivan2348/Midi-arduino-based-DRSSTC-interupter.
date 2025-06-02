#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <MIDI.h>
#include <TimerOne.h>

// Define LCD
LiquidCrystal_I2C lcd(0x27, 16, 2); // 16x2 LCD

// Pin definitions
const int upButton = 2;
const int downButton = 3;
const int selectButton = 4;
const int startStopButton = 5;
const int outputPin = 9; // Must support Timer1 (Pin 9 on Uno/Nano)

// Modes
enum Mode { SINGLE, BURST, CONTINUOUS, MIDI_MODE };
Mode currentMode = BURST;
bool isRunning = false;

// Parameters
unsigned int pulseWidth = 30;  // Default Ontime in Microseconds (Adjustable)
unsigned int frequency = 100;   // Default frequency in Hz (Adjustable)
const unsigned int pulseWidthMin = 5;
const unsigned int pulseWidthMax = 120; // Max Allowable Ontime in Microseconds, (Adjustable)
const unsigned int frequencyMin = 5;
const unsigned int frequencyMax = 500; // Max Allowable Frequency, (Adjustable)

volatile bool midiNoteActive = false;
volatile unsigned int midiNoteFrequency = 0;
byte activeMidiNote = 0;

// Burst mode parameters
unsigned int burstOnTime = 100;   // in ms
unsigned int burstOffTime = 100;  // in ms

// New MIDI pulse width parameter
unsigned int midiPulseWidth = 10; // Default for MIDI mode (Adjustable)

// Menu
enum MenuItem { MENU_MODE, MENU_PW, MENU_FREQ, MENU_BURST_ON, MENU_BURST_OFF };
MenuItem currentMenu = MENU_MODE;

// MIDI setup
MIDI_CREATE_DEFAULT_INSTANCE();

// Button states
bool upPressed = false;
bool downPressed = false;
bool selectPressed = false;
bool startStopPressed = false;

void setup() {
  Serial.begin(115200);

  // MIDI callbacks
  MIDI.setHandleNoteOn(handleNoteOn);
  MIDI.setHandleNoteOff(handleNoteOff);

  // Pin setup
  pinMode(upButton, INPUT_PULLUP);
  pinMode(downButton, INPUT_PULLUP);
  pinMode(selectButton, INPUT_PULLUP);
  pinMode(startStopButton, INPUT_PULLUP);
  pinMode(outputPin, OUTPUT);

  // LCD setup
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Interrupter Ready");

  // Timer1: start but disabled initially
  Timer1.initialize(1000000UL / frequency);  // Start with default period
  Timer1.attachInterrupt(timerISR);
  Timer1.stop();

  updateDisplay();
}

void loop() {
  // Button reads
  upPressed = !digitalRead(upButton);
  downPressed = !digitalRead(downButton);
  selectPressed = !digitalRead(selectButton);
  startStopPressed = !digitalRead(startStopButton);

  // In MIDI mode: Up/Down adjust midiPulseWidth
  if (currentMode == MIDI_MODE) {
    if (upPressed) {
      midiPulseWidth = constrain(midiPulseWidth + 5, 10, 500);
      Serial.print("MIDI Pulse Width: ");
      Serial.println(midiPulseWidth);
      updateDisplay();
      delay(150);
    }
    if (downPressed) {
      midiPulseWidth = constrain(midiPulseWidth - 5, 10, 500);
      Serial.print("MIDI Pulse Width: ");
      Serial.println(midiPulseWidth);
      updateDisplay();
      delay(150);
    }
    MIDI.read();  // Keep reading MIDI
  } else {
    // Up button (adjusts selected param)
    if (upPressed) {
      adjustValue(5); // Adjust this and the lower one to Control by how much to skip by when adjusting a selected value
      delay(150); // Simple debounce + speed
    }

    // Down button
    if (downPressed) {
      adjustValue(-5); // this is the lower one, nake sure this is negative, or pressing any button will only make the selected value go up.
      delay(150);
    }
  }

  // Select button (switch between menu items or exit MIDI mode)
  if (selectPressed) {
    if (currentMode == MIDI_MODE) {
      exitMidiMode();
    } else {
      cycleMenu();
    }
    delay(300);  // Debounce
  }

  // Start/Stop button
  if (startStopPressed) {
    toggleStartStop();
    delay(300);  // Debounce
  }
}

// ---- Update Display ----
void updateDisplay() {
  lcd.clear();
  lcd.setCursor(0, 0);

  lcd.print("Mode: ");
  lcd.print(modeToString(currentMode));
  if (currentMenu == MENU_MODE) lcd.print(" *");

  lcd.setCursor(0, 1);
  if (currentMode == MIDI_MODE) {
    lcd.print("MIDI PW:");
    lcd.print(midiPulseWidth);
    lcd.print("us");
  } else if (currentMode == BURST && (currentMenu == MENU_BURST_ON || currentMenu == MENU_BURST_OFF)) {
    // Show burst settings when selected
    if (currentMenu == MENU_BURST_ON) {
      lcd.print("Burst ON: ");
      lcd.print(burstOnTime);
      lcd.print("ms *");
    } else if (currentMenu == MENU_BURST_OFF) {
      lcd.print("Burst OFF:");
      lcd.print(burstOffTime);
      lcd.print("ms *");
    }
  } else {
    lcd.print("PW:");
    lcd.print(pulseWidth);
    lcd.print("us");
    if (currentMenu == MENU_PW) lcd.print("* ");
    else lcd.print(" ");

    lcd.print("F:");
    lcd.print(frequency);
    lcd.print("Hz");
    if (currentMenu == MENU_FREQ) lcd.print("*");
  }
}

// ---- Adjusting Values ----
void adjustValue(int direction) {
  if (currentMenu == MENU_MODE) {
    currentMode = static_cast<Mode>((currentMode + direction + 4) % 4);
    if (currentMode == MIDI_MODE) {
      Timer1.stop();
      isRunning = false;
    }
  } else if (currentMenu == MENU_PW) {
    pulseWidth = constrain(pulseWidth + direction, pulseWidthMin, pulseWidthMax);
    Serial.print("Pulse Width: ");
    Serial.println(pulseWidth);
  } else if (currentMenu == MENU_FREQ) {
    frequency = constrain(frequency + direction, frequencyMin, frequencyMax);
    Serial.print("Frequency: ");
    Serial.println(frequency);
    Timer1.setPeriod(1000000UL / frequency);
  } else if (currentMenu == MENU_BURST_ON) {
    burstOnTime = constrain(burstOnTime + (direction * 10), 10, 5000);
    Serial.print("Burst On Time: ");
    Serial.println(burstOnTime);
  } else if (currentMenu == MENU_BURST_OFF) {
    burstOffTime = constrain(burstOffTime + (direction * 10), 10, 5000);
    Serial.print("Burst Off Time: ");
    Serial.println(burstOffTime);
  }
  updateDisplay();
}

// ---- Cycle Menu Items ----
void cycleMenu() {
  if (currentMode == BURST) {
    // In BURST mode cycle through all options
    currentMenu = static_cast<MenuItem>((currentMenu + 1) % 5);
  } else {
    // In other modes skip burst-related items
    currentMenu = static_cast<MenuItem>((currentMenu + 1) % 3);
  }
  updateDisplay();
}

// ---- Toggle Start/Stop ----
void toggleStartStop() {
  if (currentMode == SINGLE) {
    Serial.println("Single Pulse Triggered");
    sendPulse();  // Manual single shot
  } else if (currentMode == BURST || currentMode == CONTINUOUS) {
    isRunning = !isRunning;
    if (isRunning) {
      Serial.println("Pulse Train Started");
      Timer1.start();
    } else {
      Serial.println("Pulse Train Stopped");
      Timer1.stop();
    }
  }
}

// ---- Timer ISR ----
void timerISR() {
  if (currentMode == BURST) {
    static unsigned long lastToggle = 0;
    static bool burstState = true;
    unsigned long now = millis();

    if (burstState && (now - lastToggle >= burstOnTime)) {
      burstState = false;
      lastToggle = now;
    } else if (!burstState && (now - lastToggle >= burstOffTime)) {
      burstState = true;
      lastToggle = now;
    }

    if (burstState) {
      sendPulse();
    }
  } else if (currentMode == CONTINUOUS) {
    sendPulse();
  } else if (currentMode == MIDI_MODE && midiNoteActive) {
    // Emit square wave at MIDI frequency
    digitalWrite(outputPin, HIGH);
    delayMicroseconds(midiPulseWidth);  // user-adjustable
    digitalWrite(outputPin, LOW);
  }
}


// ---- Send Pulse ----
void sendPulse() {
  digitalWrite(outputPin, HIGH);
  delayMicroseconds(pulseWidth);
  digitalWrite(outputPin, LOW);
}

// ---- Exit MIDI Mode ----
void exitMidiMode() {
  Serial.println("Exiting MIDI Mode");
  currentMode = BURST;
  currentMenu = MENU_MODE;
  updateDisplay();
}

// ---- Helpers ----
const char* modeToString(Mode mode) {
  switch (mode) {
    case SINGLE: return "Single";
    case BURST: return "Burst";
    case CONTINUOUS: return "Cont.";
    case MIDI_MODE: return "MIDI";
    default: return "";
  }
}

void handleNoteOn(byte channel, byte note, byte velocity) {
  if (currentMode != MIDI_MODE || velocity == 0) return;

  activeMidiNote = note;
  midiNoteFrequency = 440.0 * pow(2.0, (note - 69) / 12.0);  // A4 = 440 Hz formula
  midiNoteActive = true;

  unsigned long period = 1000000UL / midiNoteFrequency; // in microseconds
  Timer1.setPeriod(period);
  Timer1.start();

  Serial.print("Note ON: ");
  Serial.print(note);
  Serial.print(" Freq: ");
  Serial.println(midiNoteFrequency);
}


void handleNoteOff(byte channel, byte note, byte velocity) {
  if (note == activeMidiNote) {
    midiNoteActive = false;
    Timer1.stop();
    digitalWrite(outputPin, LOW);
    
    Serial.print("Note OFF: ");
    Serial.println(note);
  }
}
