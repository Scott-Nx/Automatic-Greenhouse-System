/*
 * Watchdog.h
 *
 * Watchdog Timer functions for EMI protection
 * Prevents system hang from electromagnetic interference
 *
 * Part of Automatic Greenhouse System v2.2
 */

#ifndef WATCHDOG_H
#define WATCHDOG_H

#include <Arduino.h>
#include <avr/wdt.h>
#include "Config.h"

// =============================================
// Reset Reason Enumeration
// =============================================

enum class ResetReason : uint8_t {
  POWER_ON,
  EXTERNAL,
  BROWN_OUT,
  WATCHDOG,
  UNKNOWN
};

// =============================================
// Function Declarations
// =============================================

/**
 * Initialize the Watchdog Timer
 * Call this at the end of setup() after all initialization is complete
 */
void watchdogSetup();

/**
 * Reset the Watchdog Timer
 * Must be called regularly in loop() to prevent system reset
 */
void watchdogReset();

/**
 * Disable the Watchdog Timer
 * Call at the beginning of setup() to prevent reset loops
 */
void watchdogDisable();

/**
 * Check and report the reason for the last reset
 * @return ResetReason enum indicating the cause of reset
 */
ResetReason checkResetReason();

/**
 * Get the WDT reset count (number of watchdog resets since power-on)
 * @return Number of watchdog resets
 */
uint8_t getWdtResetCount();

/**
 * Reset the WDT reset counter
 */
void clearWdtResetCount();

/**
 * Safe delay that resets watchdog during wait
 * Use this instead of delay() for long waits
 * @param ms Delay time in milliseconds
 */
void safeDelay(unsigned long ms);

#endif // WATCHDOG_H
