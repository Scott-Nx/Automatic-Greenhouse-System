/*
 * Automatic Greenhouse System - Main Program
 * For Arduino Uno
 *
 * Automatic soil moisture control system:
 * - IDLE mode: System resting when moisture is normal
 * - WATERING mode: Pump activated when soil is dry
 * - VENTILATING mode: Fan activated when soil is too wet
 *
 * Safety Features:
 * - Watchdog Timer to prevent MCU hang from EMI
 * - Strict sensor validation with median filtering
 * - Fail-safe relay shutdown on errors
 *
 * Version: 2.2 (with Watchdog and EMI Protection)
 *
 * File Structure:
 * - main.cpp        : Main program (setup/loop)
 * - Config.h        : Configuration constants
 * - Watchdog.h/cpp  : Watchdog timer functions
 * - Sensor.h/cpp    : Soil moisture sensor functions
 * - Relay.h/cpp     : Relay control functions
 * - StateMachine.h/cpp : System state management
 * - Display.h/cpp   : LCD and Serial display functions
 */

#include <Arduino.h>

// Include module headers
#include "Config.h"
#include "Watchdog.h"
#include "Sensor.h"
#include "Relay.h"
#include "StateMachine.h"
#include "Display.h"

// =============================================
// Setup Function
// =============================================

void setup() {
  // Disable Watchdog first (may be stuck from previous reset cycle)
  watchdogDisable();

  // Initialize Serial Monitor for debugging
  Serial.begin(9600);

  // Wait for Serial to be ready (required for some boards)
  unsigned long serialWaitStart = millis();
  while (!Serial && (millis() - serialWaitStart) < 3000) {
    ; // Wait up to 3 seconds
  }

  // Print startup banner
  Serial.println(F(""));
  Serial.println(F("========================================"));
  Serial.println(F("Automatic Greenhouse System v" SYSTEM_VERSION));
  Serial.println(F("With Watchdog & EMI Protection"));
  Serial.println(F("========================================"));
  Serial.println(F(""));

  // Check and display reset reason (helps diagnose WDT resets)
  checkResetReason();

  // Initialize all hardware modules
  Serial.println(F("[INIT] Initializing system..."));

  relayInit();                        // Initialize relay pins to OFF state
  sensorInit(Pins::SOIL_MOISTURE);    // Initialize soil moisture sensor
  initializeLcd();                    // Initialize I2C LCD display

  // Initialize system state data
  systemData.reset();
  systemData.stateStartTime = millis();
  systemData.lastStateChangeTime = millis();

  Serial.println(F("[INIT] System initialization complete!"));
  Serial.println(F("[STATE] Entering IDLE mode"));
  Serial.println(F(""));

  // Show startup screen on LCD
  lcdShowStartupScreen();
  safeDelay(2000);  // Display startup screen for 2 seconds (uses safe delay)

  // Enable Watchdog Timer after all initialization is complete
  // This prevents WDT reset during slow initialization
  watchdogSetup();
}

// =============================================
// Main Loop
// =============================================

void loop() {
  // Reset Watchdog Timer at start of every loop iteration
  // This tells the WDT that the system is running normally
  watchdogReset();

  unsigned long currentTime = millis();

  // Determine sensor read interval based on current state
  // Read more frequently during active states for better responsiveness
  unsigned long readInterval = (systemData.currentState == SystemState::IDLE)
                               ? Config::IDLE_READ_INTERVAL
                               : Config::READ_INTERVAL;

  // Read sensor at configured intervals
  if ((currentTime - systemData.lastReadTime) >= readInterval) {
    systemData.lastReadTime = currentTime;

    // Reset watchdog before sensor reading (median filter takes time)
    watchdogReset();

    // Store previous moisture value for EMI spike detection
    systemData.previousMoisture = systemData.currentMoisture;

    // Read moisture value from sensor (uses median filter)
    int newMoisture = readSoilMoisture();

    // Check if new reading is an EMI spike (abnormal sudden change)
    if (!isSensorError() && checkEmiSpike(newMoisture, systemData.previousMoisture)) {
      Serial.println(F("[EMI] Abnormal value detected (possible EMI) - using previous value"));
      incrementConsecutiveErrors();
    } else {
      // Valid reading - update current moisture
      systemData.currentMoisture = newMoisture;
      if (!isSensorError()) {
        resetConsecutiveErrors();  // Clear error count on valid reading
      }
    }

    // Sync error tracking with sensor module
    systemData.sensorError = isSensorError();
    systemData.consecutiveErrors = getConsecutiveErrors();

    // Reset watchdog after sensor processing
    watchdogReset();

    // Output current status to Serial Monitor
    printSystemStatus(systemData.currentMoisture);

    // Check for excessive consecutive errors
    if (systemData.consecutiveErrors >= WatchdogConfig::MAX_CONSECUTIVE_ERRORS) {
      Serial.println(F("[ERROR] Too many consecutive sensor errors!"));
      handleError();  // Enter safe ERROR state
    } else if (!systemData.sensorError && systemData.currentState != SystemState::ERROR) {
      // Normal operation - update system state based on moisture level
      updateSystemState(systemData.currentMoisture);
    }
  }

  // Reset watchdog between operations
  watchdogReset();

  // Update LCD display at configured intervals
  if ((currentTime - systemData.lastLcdUpdateTime) >= Config::LCD_UPDATE_INTERVAL) {
    systemData.lastLcdUpdateTime = currentTime;
    updateLcdDisplay();
  }

  // Reset watchdog before state execution
  watchdogReset();

  // Execute actions for current system state (pump/fan control)
  executeState();

  // Final watchdog reset at end of loop
  watchdogReset();
}
