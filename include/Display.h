/*
 * Display.h
 * ฟังก์ชันแสดงผล LCD และ Serial
 *
 * Automatic Greenhouse System v2.2
 */

#ifndef DISPLAY_H
#define DISPLAY_H

#include <Arduino.h>
#include <LiquidCrystal_I2C.h>
#include "Config.h"

// Forward declaration to avoid circular dependency
enum class SystemState : uint8_t;

// =============================================
// Custom LCD Characters (ตัวอักษรพิเศษ)
// =============================================

namespace LcdIcons {
  constexpr uint8_t WATER_DROP = 0;
  constexpr uint8_t FAN        = 1;
  constexpr uint8_t PLANT      = 2;
  constexpr uint8_t WARNING    = 3;

  extern const uint8_t CHAR_WATER_DROP[8] PROGMEM;
  extern const uint8_t CHAR_FAN[8] PROGMEM;
  extern const uint8_t CHAR_PLANT[8] PROGMEM;
  extern const uint8_t CHAR_WARNING[8] PROGMEM;
}

// =============================================
// LCD Functions
// =============================================

// Initialize LCD display
void initializeLcd();

// Create custom characters for LCD
void createLcdCustomChars();

// Show startup screen on LCD
void lcdShowStartupScreen();

// Update LCD display based on current state
void updateLcdDisplay();

// Show system status on LCD
void lcdShowSystemStatus();

// Show error screen on LCD
void lcdShowErrorScreen();

// Clear a specific row on LCD
void lcdClearRow(uint8_t row);

// Get short state name for LCD display
const char* getLcdStateName(SystemState state);

// Get short moisture status for LCD display
const char* getLcdMoistureStatus(int moisture);

// Convert raw moisture value to percentage
int getMoisturePercent(int rawValue);

// =============================================
// Serial Display Functions
// =============================================

// Print system status to Serial
void printSystemStatus(int moisture);

// Print state transition to Serial
void printStateTransition(SystemState from, SystemState to);

// Get state name as flash string
const __FlashStringHelper* getStateName(SystemState state);

// Get moisture status as flash string
const __FlashStringHelper* getMoistureStatus(int moisture);

// =============================================
// External LCD Object (defined in Display.cpp)
// =============================================

extern LiquidCrystal_I2C lcd;

#endif // DISPLAY_H
