/*
 * ระบบโรงเรือนอัตโนมัติ (Automatic Greenhouse System)
 * สำหรับบอร์ด Arduino Uno
 *
 * ระบบควบคุมความชื้นในดินอัตโนมัติ:
 * - เมื่อความชื้นปกติ: โหมด IDLE (พักระบบ)
 * - เมื่อความชื้นต่ำ: เปิดปั๊มน้ำเพื่อรดน้ำ
 * - เมื่อความชื้นสูง: เปิดพัดลมเพื่อดูดความชื้นออก
 *
 * อุปกรณ์แสดงผล:
 * - LCD Display 16x2 (I2C) สำหรับแสดงสถานะระบบ
 *
 * คุณสมบัติด้านความปลอดภัย:
 * - Watchdog Timer เพื่อป้องกันการค้างจาก EMI
 * - การตรวจสอบ Sensor อย่างเข้มงวด
 * - ระบบ Fail-safe สำหรับอุปกรณ์
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
  // Disable Watchdog first (may be stuck from previous cycle)
  watchdogDisable();

  // Initialize Serial Monitor for debugging
  Serial.begin(9600);

  // Wait for Serial to be ready (for some boards)
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

  // Check reset reason
  checkResetReason();

  // Initialize all modules
  Serial.println(F("[INIT] Initializing system..."));

  relayInit();                        // Initialize relay pins
  sensorInit(Pins::SOIL_MOISTURE);    // Initialize sensor
  initializeLcd();                    // Initialize LCD display

  // Initialize system data
  systemData.reset();
  systemData.stateStartTime = millis();
  systemData.lastStateChangeTime = millis();

  Serial.println(F("[INIT] System initialization complete!"));
  Serial.println(F("[STATE] Entering IDLE mode"));
  Serial.println(F(""));

  // Show startup screen on LCD
  lcdShowStartupScreen();
  safeDelay(2000);  // Show startup screen for 2 seconds

  // Start Watchdog Timer after setup is complete
  watchdogSetup();
}

// =============================================
// Main Loop
// =============================================

void loop() {
  // Reset Watchdog Timer every loop iteration
  watchdogReset();

  unsigned long currentTime = millis();

  // Determine read interval based on current state
  unsigned long readInterval = (systemData.currentState == SystemState::IDLE)
                               ? Config::IDLE_READ_INTERVAL
                               : Config::READ_INTERVAL;

  // Read sensor at specified intervals
  if ((currentTime - systemData.lastReadTime) >= readInterval) {
    systemData.lastReadTime = currentTime;

    // Reset watchdog before sensor reading (may take time)
    watchdogReset();

    // Save previous moisture value
    systemData.previousMoisture = systemData.currentMoisture;

    // Read moisture from sensor
    int newMoisture = readSoilMoisture();

    // Check for EMI spike
    if (!isSensorError() && checkEmiSpike(newMoisture, systemData.previousMoisture)) {
      Serial.println(F("[EMI] Abnormal value detected (possible EMI) - using previous value"));
      incrementConsecutiveErrors();
    } else {
      systemData.currentMoisture = newMoisture;
      if (!isSensorError()) {
        resetConsecutiveErrors();  // Reset error count on success
      }
    }

    // Update systemData error tracking
    systemData.sensorError = isSensorError();
    systemData.consecutiveErrors = getConsecutiveErrors();

    // Reset watchdog after sensor reading
    watchdogReset();

    // Print system status to Serial
    printSystemStatus(systemData.currentMoisture);

    // Check for too many consecutive errors
    if (systemData.consecutiveErrors >= WatchdogConfig::MAX_CONSECUTIVE_ERRORS) {
      Serial.println(F("[ERROR] Too many consecutive errors!"));
      handleError();
    } else if (!systemData.sensorError && systemData.currentState != SystemState::ERROR) {
      // Update system state (if no errors)
      updateSystemState(systemData.currentMoisture);
    }
  }

  // Reset watchdog
  watchdogReset();

  // Update LCD at specified intervals
  if ((currentTime - systemData.lastLcdUpdateTime) >= Config::LCD_UPDATE_INTERVAL) {
    systemData.lastLcdUpdateTime = currentTime;
    updateLcdDisplay();
  }

  // Reset watchdog
  watchdogReset();

  // Execute actions for current state
  executeState();

  // Reset watchdog at end of loop
  watchdogReset();
}
