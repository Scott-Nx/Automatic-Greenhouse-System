/*
 * Display.cpp
 * ฟังก์ชันแสดงผล LCD และ Serial
 *
 * Automatic Greenhouse System v2.2
 */

#include "Display.h"
#include "StateMachine.h"
#include "Sensor.h"

// =============================================
// LCD Object
// =============================================

LiquidCrystal_I2C lcd(LcdConfig::I2C_ADDRESS, LcdConfig::COLUMNS, LcdConfig::ROWS);

// =============================================
// Custom Character Definitions
// =============================================

namespace LcdIcons {
  const uint8_t CHAR_WATER_DROP[8] PROGMEM = {
    0b00100,
    0b00100,
    0b01110,
    0b01110,
    0b11111,
    0b11111,
    0b11111,
    0b01110
  };

  const uint8_t CHAR_FAN[8] PROGMEM = {
    0b00000,
    0b11011,
    0b11011,
    0b00100,
    0b11011,
    0b11011,
    0b00000,
    0b00000
  };

  const uint8_t CHAR_PLANT[8] PROGMEM = {
    0b00100,
    0b01110,
    0b00100,
    0b01110,
    0b10101,
    0b00100,
    0b00100,
    0b01110
  };

  const uint8_t CHAR_WARNING[8] PROGMEM = {
    0b00000,
    0b00100,
    0b01110,
    0b01110,
    0b11111,
    0b11111,
    0b00100,
    0b00000
  };
}

// =============================================
// LCD Initialization
// =============================================

void initializeLcd() {
  // Initialize LCD
  lcd.init();

  // Turn on backlight
  lcd.backlight();

  // Create custom characters
  createLcdCustomChars();

  // Clear screen
  lcd.clear();

  Serial.println(F("[LCD] Initialized (16x2 I2C)"));
}

void createLcdCustomChars() {
  uint8_t charBuffer[8];

  // Load and create water drop icon
  memcpy_P(charBuffer, LcdIcons::CHAR_WATER_DROP, 8);
  lcd.createChar(LcdIcons::WATER_DROP, charBuffer);

  // Load and create fan icon
  memcpy_P(charBuffer, LcdIcons::CHAR_FAN, 8);
  lcd.createChar(LcdIcons::FAN, charBuffer);

  // Load and create plant icon
  memcpy_P(charBuffer, LcdIcons::CHAR_PLANT, 8);
  lcd.createChar(LcdIcons::PLANT, charBuffer);

  // Load and create warning icon
  memcpy_P(charBuffer, LcdIcons::CHAR_WARNING, 8);
  lcd.createChar(LcdIcons::WARNING, charBuffer);
}

// =============================================
// LCD Display Functions
// =============================================

void lcdShowStartupScreen() {
  lcd.clear();

  // Row 1: System name
  lcd.setCursor(0, 0);
  lcd.write(LcdIcons::PLANT);
  lcd.print(F(" Greenhouse"));

  // Row 2: Version
  lcd.setCursor(0, 1);
  lcd.print(F("System v" SYSTEM_VERSION " WDT"));
}

void updateLcdDisplay() {
  if (systemData.currentState == SystemState::ERROR) {
    lcdShowErrorScreen();
  } else {
    lcdShowSystemStatus();
  }
}

void lcdShowErrorScreen() {
  lcd.setCursor(0, 0);
  lcd.write(LcdIcons::WARNING);
  lcd.print(F(" SENSOR ERROR  "));

  lcd.setCursor(0, 1);
  lcd.print(F("Check connection"));
}

void lcdShowSystemStatus() {
  // Row 1: Moisture value and status
  // Format: "XM:xxx% STATUS"
  lcd.setCursor(0, 0);

  // Show icon based on state
  if (systemData.sensorError) {
    lcd.write(LcdIcons::WARNING);
  } else if (systemData.currentState == SystemState::WATERING) {
    lcd.write(LcdIcons::WATER_DROP);
  } else if (systemData.currentState == SystemState::VENTILATING) {
    lcd.write(LcdIcons::FAN);
  } else {
    lcd.write(LcdIcons::PLANT);
  }

  // Show moisture percentage
  lcd.print(F("M:"));
  int moisturePercent = getMoisturePercent(systemData.currentMoisture);

  // Format number (pad with spaces)
  if (moisturePercent < 10) {
    lcd.print(F("  "));
  } else if (moisturePercent < 100) {
    lcd.print(F(" "));
  }
  lcd.print(moisturePercent);
  lcd.print(F("% "));

  // Show short moisture status
  lcd.print(getLcdMoistureStatus(systemData.currentMoisture));

  // Pad remaining spaces
  lcd.print(F("   "));

  // Row 2: System state and time
  // Format: "STATE    xxxs"
  lcd.setCursor(0, 1);

  // Show system state
  lcd.print(getLcdStateName(systemData.currentState));

  // Show elapsed time in current state
  unsigned long elapsed = millis() - systemData.stateStartTime;

  // Handle millis() overflow
  if (millis() < systemData.stateStartTime) {
    elapsed = (0xFFFFFFFFUL - systemData.stateStartTime) + millis() + 1;
  }

  unsigned long elapsedSec = elapsed / 1000;

  // Position for time display (right-aligned)
  lcd.setCursor(11, 1);

  // Format number (pad with spaces)
  if (elapsedSec < 10) {
    lcd.print(F("   "));
  } else if (elapsedSec < 100) {
    lcd.print(F("  "));
  } else if (elapsedSec < 1000) {
    lcd.print(F(" "));
  }
  lcd.print(elapsedSec);
  lcd.print(F("s"));
}

void lcdClearRow(uint8_t row) {
  lcd.setCursor(0, row);
  lcd.print(F("                "));  // 16 spaces
}

const char* getLcdStateName(SystemState state) {
  // Short state names for LCD (max 10 characters)
  switch (state) {
    case SystemState::IDLE:        return "IDLE      ";
    case SystemState::WATERING:    return "WATERING  ";
    case SystemState::VENTILATING: return "VENT      ";
    case SystemState::COOLDOWN:    return "COOLDOWN  ";
    case SystemState::ERROR:       return "ERROR     ";
    default:                       return "UNKNOWN   ";
  }
}

const char* getLcdMoistureStatus(int moisture) {
  // Short moisture status for LCD
  if (moisture >= Config::MOISTURE_DRY_THRESHOLD) {
    return "DRY";
  } else if (moisture <= Config::MOISTURE_WET_THRESHOLD) {
    return "WET";
  } else {
    return "OK ";
  }
}

// =============================================
// Serial Display Functions
// =============================================

void printSystemStatus(int moisture) {
  Serial.println(F("-------------------------------------"));

  // Show moisture value
  Serial.print(F("Moisture: "));
  Serial.print(moisture);
  Serial.print(F(" ("));
  Serial.print(getMoisturePercent(moisture));
  Serial.print(F("%)"));
  Serial.print(F(" | Status: "));
  Serial.println(getMoistureStatus(moisture));

  // Show system state
  Serial.print(F("System State: "));
  Serial.print(getStateName(systemData.currentState));

  // Show time in current state
  unsigned long elapsed = millis() - systemData.stateStartTime;
  if (millis() < systemData.stateStartTime) {
    elapsed = (0xFFFFFFFFUL - systemData.stateStartTime) + millis() + 1;
  }

  Serial.print(F(" ("));
  Serial.print(elapsed / 1000);
  Serial.println(F("s)"));

  // Show device status
  Serial.print(F("Pump: "));
  Serial.print(systemData.currentState == SystemState::WATERING ? F("ON") : F("OFF"));
  Serial.print(F(" | Fan: "));
  Serial.println(systemData.currentState == SystemState::VENTILATING ? F("ON") : F("OFF"));

  // Show error info
  if (systemData.sensorError) {
    Serial.println(F("!!! SENSOR ERROR - Using previous value !!!"));
  }
  if (systemData.consecutiveErrors > 0) {
    Serial.print(F("[WARN] Consecutive errors: "));
    Serial.println(systemData.consecutiveErrors);
  }

  Serial.println(F("-------------------------------------"));
  Serial.println(F(""));
}

void printStateTransition(SystemState from, SystemState to) {
  Serial.println(F(""));
  Serial.print(F("==> STATE CHANGE: "));
  Serial.print(getStateName(from));
  Serial.print(F(" -> "));
  Serial.println(getStateName(to));
}

const __FlashStringHelper* getStateName(SystemState state) {
  switch (state) {
    case SystemState::IDLE:        return F("IDLE");
    case SystemState::WATERING:    return F("WATERING");
    case SystemState::VENTILATING: return F("VENTILATING");
    case SystemState::COOLDOWN:    return F("COOLDOWN");
    case SystemState::ERROR:       return F("ERROR");
    default:                       return F("UNKNOWN");
  }
}

const __FlashStringHelper* getMoistureStatus(int moisture) {
  if (moisture >= Config::MOISTURE_DRY_THRESHOLD) {
    return F("DRY (Soil is dry)");
  } else if (moisture <= Config::MOISTURE_WET_THRESHOLD) {
    return F("TOO WET (Too much moisture)");
  } else {
    return F("NORMAL (OK)");
  }
}
