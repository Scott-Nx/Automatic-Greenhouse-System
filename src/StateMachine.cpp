/*
 * StateMachine.cpp
 * ระบบจัดการสถานะ (State Machine)
 *
 * Automatic Greenhouse System v2.2
 */

#include "StateMachine.h"
#include "Config.h"
#include "Relay.h"
#include "Watchdog.h"
#include "Display.h"

// =============================================
// Global System Data Definition
// =============================================

SystemData systemData;

// =============================================
// Helper Function
// =============================================

static unsigned long getElapsedTime(unsigned long startTime) {
  unsigned long currentTime = millis();
  // Handle millis() overflow (approximately 49 days)
  if (currentTime >= startTime) {
    return currentTime - startTime;
  } else {
    // Overflow case
    return (0xFFFFFFFFUL - startTime) + currentTime + 1;
  }
}

// =============================================
// State Name Function
// =============================================

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

// =============================================
// Update System State
// =============================================

void updateSystemState(int moisture) {
  switch (systemData.currentState) {
    case SystemState::IDLE:
      // Check if state change is needed
      if (moisture >= Config::MOISTURE_DRY_THRESHOLD) {
        transitionTo(SystemState::WATERING);
      } else if (moisture <= Config::MOISTURE_WET_THRESHOLD) {
        transitionTo(SystemState::VENTILATING);
      }
      // If moisture is normal, stay in IDLE
      break;

    case SystemState::WATERING:
      // Check if moisture has improved (with hysteresis)
      if (moisture < (Config::MOISTURE_DRY_THRESHOLD - Config::HYSTERESIS)) {
        transitionTo(SystemState::COOLDOWN);
      }
      break;

    case SystemState::VENTILATING:
      // Check if moisture has decreased (with hysteresis)
      if (moisture > (Config::MOISTURE_WET_THRESHOLD + Config::HYSTERESIS)) {
        transitionTo(SystemState::COOLDOWN);
      }
      break;

    case SystemState::COOLDOWN:
      // Handled in executeState()
      break;

    case SystemState::ERROR:
      // Handled in handleError()
      break;
  }
}

// =============================================
// Execute State
// =============================================

void executeState() {
  unsigned long elapsed = getElapsedTime(systemData.stateStartTime);

  switch (systemData.currentState) {
    case SystemState::IDLE:
      // Nothing to do - system is idle
      break;

    case SystemState::WATERING:
      // Check pump run time
      if (elapsed >= Config::PUMP_RUN_TIME) {
        Serial.println(F("[PUMP] Stopping pump (time complete)"));
        transitionTo(SystemState::COOLDOWN);
      }
      break;

    case SystemState::VENTILATING:
      // Check fan run time
      if (elapsed >= Config::FAN_RUN_TIME) {
        Serial.println(F("[FAN] Stopping fan (time complete)"));
        transitionTo(SystemState::COOLDOWN);
      }
      break;

    case SystemState::COOLDOWN:
      // Check if cooldown time has elapsed
      if (elapsed >= Config::COOLDOWN_TIME) {
        Serial.println(F("[COOLDOWN] Cooldown complete"));
        transitionTo(SystemState::IDLE);
      }
      break;

    case SystemState::ERROR:
      // In error state, wait for sensor to recover
      if (!systemData.sensorError && systemData.consecutiveErrors == 0) {
        Serial.println(F("[RECOVERY] Sensor recovered - returning to normal operation"));
        transitionTo(SystemState::IDLE);
      }
      break;
  }
}

// =============================================
// Transition To New State
// =============================================

void transitionTo(SystemState newState) {
  if (systemData.currentState == newState) {
    return; // No change
  }

  // Save old state
  systemData.previousState = systemData.currentState;

  // Stop all devices first
  relayStopAll();

  // Print state transition
  printStateTransition(systemData.currentState, newState);

  // Change state
  systemData.currentState = newState;
  systemData.stateStartTime = millis();
  systemData.lastStateChangeTime = millis();

  // Reset watchdog after state change
  watchdogReset();

  // Execute actions for new state
  switch (newState) {
    case SystemState::IDLE:
      Serial.println(F("[STATE] Entering IDLE mode"));
      break;

    case SystemState::WATERING:
      pumpStart();
      break;

    case SystemState::VENTILATING:
      fanStart();
      break;

    case SystemState::COOLDOWN:
      Serial.println(F("[STATE] Entering COOLDOWN period"));
      break;

    case SystemState::ERROR:
      Serial.println(F("[STATE] Entering ERROR mode - all devices stopped"));
      relaySafeShutdown();
      break;
  }

  // Update LCD immediately on state change
  updateLcdDisplay();
}

// =============================================
// Handle Error
// =============================================

void handleError() {
  Serial.println(F("[ERROR] System entering Safe Mode"));

  // Stop all devices for safety
  relaySafeShutdown();

  // Transition to error state
  transitionTo(SystemState::ERROR);
}
