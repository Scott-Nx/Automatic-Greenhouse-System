/*
 * Relay.cpp
 * Implementation of Relay control functions
 *
 * Automatic Greenhouse System v2.2
 */

#include "Relay.h"

// =============================================
// Internal State Tracking
// =============================================

static bool pumpRunning = false;
static bool fanRunning = false;
static bool relay1Active = false;
static bool relay2Active = false;

// =============================================
// Initialization
// =============================================

void relayInit() {
  // Set relay pins as OUTPUT
  pinMode(Pins::RELAY_1, OUTPUT);
  pinMode(Pins::RELAY_2, OUTPUT);
  pinMode(Pins::RELAY_PUMP, OUTPUT);
  pinMode(Pins::RELAY_FAN, OUTPUT);

  // Turn off all relays initially (Active-Low: HIGH = OFF)
  digitalWrite(Pins::RELAY_1, RelayState::OFF);
  digitalWrite(Pins::RELAY_2, RelayState::OFF);
  digitalWrite(Pins::RELAY_PUMP, RelayState::OFF);
  digitalWrite(Pins::RELAY_FAN, RelayState::OFF);

  // Reset state tracking
  pumpRunning = false;
  fanRunning = false;
  relay1Active = false;
  relay2Active = false;

  Serial.println(F("[RELAY] Initialized - All relays OFF"));
}

// =============================================
// Pump Control Functions
// =============================================

void pumpStart() {
  if (!pumpRunning) {
    Serial.println(F(""));
    Serial.println(F(">>> [PUMP] Starting water pump..."));
    digitalWrite(Pins::RELAY_PUMP, RelayState::ON);
    pumpRunning = true;
  }
}

void pumpStop() {
  if (pumpRunning) {
    digitalWrite(Pins::RELAY_PUMP, RelayState::OFF);
    pumpRunning = false;
    Serial.println(F("[PUMP] Stopped"));
  } else {
    // Ensure relay is off even if state tracking is wrong
    digitalWrite(Pins::RELAY_PUMP, RelayState::OFF);
  }
}

bool pumpIsRunning() {
  return pumpRunning;
}

// =============================================
// Fan Control Functions
// =============================================

void fanStart() {
  if (!fanRunning) {
    Serial.println(F(""));
    Serial.println(F(">>> [FAN] Starting ventilation fan..."));
    digitalWrite(Pins::RELAY_FAN, RelayState::ON);
    fanRunning = true;
  }
}

void fanStop() {
  if (fanRunning) {
    digitalWrite(Pins::RELAY_FAN, RelayState::OFF);
    fanRunning = false;
    Serial.println(F("[FAN] Stopped"));
  } else {
    // Ensure relay is off even if state tracking is wrong
    digitalWrite(Pins::RELAY_FAN, RelayState::OFF);
  }
}

bool fanIsRunning() {
  return fanRunning;
}

// =============================================
// Reserved Relay 1 Functions
// =============================================

void relay1Activate() {
  if (!relay1Active) {
    digitalWrite(Pins::RELAY_1, RelayState::ON);
    relay1Active = true;
    Serial.println(F("[RELAY1] Activated"));
  }
}

void relay1Deactivate() {
  if (relay1Active) {
    digitalWrite(Pins::RELAY_1, RelayState::OFF);
    relay1Active = false;
    Serial.println(F("[RELAY1] Deactivated"));
  } else {
    digitalWrite(Pins::RELAY_1, RelayState::OFF);
  }
}

bool relay1IsActive() {
  return relay1Active;
}

// =============================================
// Reserved Relay 2 Functions
// =============================================

void relay2Activate() {
  if (!relay2Active) {
    digitalWrite(Pins::RELAY_2, RelayState::ON);
    relay2Active = true;
    Serial.println(F("[RELAY2] Activated"));
  }
}

void relay2Deactivate() {
  if (relay2Active) {
    digitalWrite(Pins::RELAY_2, RelayState::OFF);
    relay2Active = false;
    Serial.println(F("[RELAY2] Deactivated"));
  } else {
    digitalWrite(Pins::RELAY_2, RelayState::OFF);
  }
}

bool relay2IsActive() {
  return relay2Active;
}

// =============================================
// Stop All Devices
// =============================================

void relayStopAll() {
  pumpStop();
  fanStop();
}

// =============================================
// Safe Shutdown (Emergency Stop)
// =============================================

void relaySafeShutdown() {
  // Force all relays OFF regardless of state
  digitalWrite(Pins::RELAY_1, RelayState::OFF);
  digitalWrite(Pins::RELAY_2, RelayState::OFF);
  digitalWrite(Pins::RELAY_PUMP, RelayState::OFF);
  digitalWrite(Pins::RELAY_FAN, RelayState::OFF);

  // Update state tracking
  pumpRunning = false;
  fanRunning = false;
  relay1Active = false;
  relay2Active = false;

  Serial.println(F("[SAFE] Emergency shutdown - All relays OFF"));
}
