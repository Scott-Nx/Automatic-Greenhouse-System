/*
 * Watchdog.h - Watchdog Timer Functions
 *
 * Provides Watchdog Timer (WDT) functionality for EMI protection.
 * The WDT automatically resets the MCU if the program hangs due to
 * electromagnetic interference or other issues.
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
  POWER_ON,    // Normal power-on reset
  EXTERNAL,    // External reset (reset button pressed)
  BROWN_OUT,   // Brown-out reset (voltage dropped too low)
  WATCHDOG,    // Watchdog timer reset (program hung)
  UNKNOWN      // Unknown reset reason
};

// =============================================
// Function Declarations
// =============================================

/**
 * Initialize the Watchdog Timer
 *
 * Configures the WDT with the timeout specified in Config.h
 * (WatchdogConfig::TIMEOUT). Call this at the END of setup()
 * after all other initialization is complete.
 *
 * Default timeout: 2 seconds (WDTO_2S)
 */
void watchdogSetup();

/**
 * Reset the Watchdog Timer
 *
 * Must be called regularly in loop() to prevent system reset.
 * If this function is not called within the timeout period,
 * the MCU will automatically reset.
 *
 * Call this:
 * - At the start of loop()
 * - Before and after long operations
 * - At the end of loop()
 */
void watchdogReset();

/**
 * Disable the Watchdog Timer
 *
 * Call this at the BEGINNING of setup() to prevent reset loops
 * if the WDT was active from a previous reset cycle.
 */
void watchdogDisable();

/**
 * Check and report the reason for the last reset
 *
 * Reads the MCU Status Register (MCUSR) to determine what
 * caused the last reset. Useful for diagnosing WDT resets.
 *
 * @return ResetReason enum indicating the cause of reset
 */
ResetReason checkResetReason();

/**
 * Get the WDT reset count
 *
 * Returns the number of watchdog resets since power-on.
 * This counter is cleared on power-on reset but survives
 * software resets.
 *
 * @return Number of watchdog resets (0-255)
 */
uint8_t getWdtResetCount();

/**
 * Clear the WDT reset counter
 *
 * Resets the watchdog reset counter to zero.
 * Call this after recovering from multiple WDT resets.
 */
void clearWdtResetCount();

/**
 * Safe delay that resets watchdog during wait
 *
 * Use this instead of delay() for waits longer than the
 * WDT timeout period. This function resets the watchdog
 * every 10ms during the delay.
 *
 * @param ms Delay time in milliseconds
 */
void safeDelay(unsigned long ms);

#endif // WATCHDOG_H
