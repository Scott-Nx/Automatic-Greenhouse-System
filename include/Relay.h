/*
 * Relay.h
 * ฟังก์ชันควบคุม Relay สำหรับระบบโรงเรือนอัตโนมัติ
 *
 * รองรับ Relay 4-Channel (Active-Low)
 */

#ifndef RELAY_H
#define RELAY_H

#include <Arduino.h>
#include "Config.h"

// =============================================
// Relay Control Functions
// =============================================

// Initialize relay pins
void relayInit();

// Pump Control
void pumpStart();
void pumpStop();
bool pumpIsRunning();

// Fan Control
void fanStart();
void fanStop();
bool fanIsRunning();

// Reserved Relay 1
void relay1Activate();
void relay1Deactivate();
bool relay1IsActive();

// Reserved Relay 2
void relay2Activate();
void relay2Deactivate();
bool relay2IsActive();

// Stop all devices
void relayStopAll();

// Safe shutdown (emergency stop)
void relaySafeShutdown();

#endif // RELAY_H
