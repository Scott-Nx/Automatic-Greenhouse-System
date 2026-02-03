/*
 * Watchdog.cpp - Watchdog Timer Implementation
 *
 * Provides Watchdog Timer (WDT) functionality for EMI protection.
 * The WDT automatically resets the MCU if the program hangs due to
 * electromagnetic interference or other issues.
 *
 * Part of Automatic Greenhouse System v2.2
 */

#include "Watchdog.h"

// =============================================
// Static Variables
// =============================================

// WDT reset counter (survives software reset, cleared on power-on)
static uint8_t wdtResetCounter = 0;

// Flag to track if we've already read MCUSR
static bool mcusrRead = false;

// Saved MCUSR value from startup
static uint8_t savedMcusr = 0;

// =============================================
// Watchdog Setup
// =============================================

void watchdogSetup() {
  // Disable interrupts during WDT configuration
  cli();

  // Clear the watchdog reset flag to prevent immediate reset
  MCUSR &= ~(1 << WDRF);

  // Enable Watchdog Change Enable (WDCE) and Watchdog Enable (WDE)
  // This allows us to modify WDT settings within the next 4 clock cycles
  WDTCSR |= (1 << WDCE) | (1 << WDE);

  // Configure watchdog timeout to 2 seconds
  // WDP3=0, WDP2=1, WDP1=1, WDP0=1 = 2s timeout
  // See Config.h WatchdogConfig::TIMEOUT for available options
  WDTCSR = (1 << WDE) | (1 << WDP2) | (1 << WDP1) | (1 << WDP0);

  // Re-enable interrupts
  sei();

  Serial.println(F("[WDT] Watchdog Timer initialized (2s timeout)"));
}

// =============================================
// Watchdog Reset
// =============================================

void watchdogReset() {
  // Reset the watchdog timer counter
  // This must be called regularly to prevent MCU reset
  wdt_reset();
}

// =============================================
// Watchdog Disable
// =============================================

void watchdogDisable() {
  // Disable interrupts for atomic operation
  cli();

  // Reset WDT first to prevent timeout during disable sequence
  wdt_reset();

  // Clear WDRF (Watchdog Reset Flag) in MCUSR
  // This is required before disabling WDT
  MCUSR &= ~(1 << WDRF);

  // Write logical one to WDCE and WDE
  // This starts the 4-cycle timed sequence to modify WDT
  WDTCSR |= (1 << WDCE) | (1 << WDE);

  // Turn off WDT by writing 0 to all bits
  WDTCSR = 0x00;

  // Re-enable interrupts
  sei();
}

// =============================================
// Check Reset Reason
// =============================================

ResetReason checkResetReason() {
  // Read MCUSR only once at startup
  if (!mcusrRead) {
    savedMcusr = MCUSR;
    MCUSR = 0;  // Clear for next reset detection
    mcusrRead = true;
  }

  ResetReason reason = ResetReason::UNKNOWN;

  Serial.print(F("[BOOT] Reset reason: "));

  // Check reset flags in priority order
  if (savedMcusr & (1 << WDRF)) {
    // Watchdog Reset Flag is set
    Serial.println(F("WATCHDOG RESET!"));
    Serial.println(F("[WARN] System reset by Watchdog - possible EMI or program hang"));
    wdtResetCounter++;
    reason = ResetReason::WATCHDOG;

    // Warn if multiple WDT resets have occurred
    if (wdtResetCounter >= 3) {
      Serial.println(F("[WARN] Multiple WDT resets detected - check for EMI issues"));
    }
  } else if (savedMcusr & (1 << BORF)) {
    // Brown-Out Reset Flag
    Serial.println(F("BROWN-OUT RESET"));
    Serial.println(F("[INFO] Power supply voltage dropped below threshold"));
    reason = ResetReason::BROWN_OUT;
  } else if (savedMcusr & (1 << EXTRF)) {
    // External Reset Flag (reset button or pin)
    Serial.println(F("EXTERNAL RESET"));
    reason = ResetReason::EXTERNAL;
  } else if (savedMcusr & (1 << PORF)) {
    // Power-On Reset Flag
    Serial.println(F("POWER-ON RESET"));
    wdtResetCounter = 0;  // Clear WDT counter on fresh power-on
    reason = ResetReason::POWER_ON;
  } else {
    // No recognized reset flag
    Serial.println(F("UNKNOWN"));
    reason = ResetReason::UNKNOWN;
  }

  // Print raw MCUSR value for debugging
  Serial.print(F("[BOOT] MCUSR value: 0x"));
  Serial.println(savedMcusr, HEX);

  return reason;
}

// =============================================
// WDT Reset Count Management
// =============================================

uint8_t getWdtResetCount() {
  return wdtResetCounter;
}

void clearWdtResetCount() {
  wdtResetCounter = 0;
}

// =============================================
// Safe Delay
// =============================================

void safeDelay(unsigned long ms) {
  // This function provides a delay that resets the watchdog periodically
  // Use this instead of delay() for waits longer than WDT timeout

  unsigned long startTime = millis();

  while ((millis() - startTime) < ms) {
    // Reset watchdog to prevent timeout
    watchdogReset();

    // Small delay to avoid tight loop consuming CPU
    // 10ms intervals ensure WDT is reset well within timeout
    delay(10);
  }
}
