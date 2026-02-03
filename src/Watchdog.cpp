/*
 * Watchdog.cpp
 *
 * Watchdog Timer implementation for EMI protection
 * Prevents system hang from electromagnetic interference
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
static uint8_t savedMcusr = 0;

// =============================================
// Watchdog Setup
// =============================================

void watchdogSetup() {
  // Disable interrupts during WDT setup
  cli();

  // Clear the reset flag
  MCUSR &= ~(1 << WDRF);

  // Set WDCE (Watchdog Change Enable) and WDE (Watchdog Enable)
  // This allows us to change WDT settings in the next 4 clock cycles
  WDTCSR |= (1 << WDCE) | (1 << WDE);

  // Set new watchdog timeout value (2 seconds)
  // WDP3=0, WDP2=1, WDP1=1, WDP0=1 = 2s timeout
  WDTCSR = (1 << WDE) | (1 << WDP2) | (1 << WDP1) | (1 << WDP0);

  // Re-enable interrupts
  sei();

  Serial.println(F("[WDT] Watchdog Timer initialized (2s timeout)"));
}

// =============================================
// Watchdog Reset
// =============================================

void watchdogReset() {
  wdt_reset();
}

// =============================================
// Watchdog Disable
// =============================================

void watchdogDisable() {
  cli();
  wdt_reset();

  // Clear WDRF in MCUSR
  MCUSR &= ~(1 << WDRF);

  // Write logical one to WDCE and WDE
  // Keep old prescaler setting to prevent unintentional time-out
  WDTCSR |= (1 << WDCE) | (1 << WDE);

  // Turn off WDT
  WDTCSR = 0x00;

  sei();
}

// =============================================
// Check Reset Reason
// =============================================

ResetReason checkResetReason() {
  // Read MCUSR only once
  if (!mcusrRead) {
    savedMcusr = MCUSR;
    MCUSR = 0;  // Clear for next time
    mcusrRead = true;
  }

  ResetReason reason = ResetReason::UNKNOWN;

  Serial.print(F("[BOOT] Reset reason: "));

  if (savedMcusr & (1 << WDRF)) {
    Serial.println(F("WATCHDOG RESET!"));
    Serial.println(F("[WARN] System reset by Watchdog - possible EMI or program hang"));
    wdtResetCounter++;
    reason = ResetReason::WATCHDOG;

    if (wdtResetCounter >= 3) {
      Serial.println(F("[WARN] Multiple WDT resets detected - entering Safe Mode"));
    }
  } else if (savedMcusr & (1 << BORF)) {
    Serial.println(F("BROWN-OUT RESET"));
    reason = ResetReason::BROWN_OUT;
  } else if (savedMcusr & (1 << EXTRF)) {
    Serial.println(F("EXTERNAL RESET"));
    reason = ResetReason::EXTERNAL;
  } else if (savedMcusr & (1 << PORF)) {
    Serial.println(F("POWER-ON RESET"));
    wdtResetCounter = 0;  // Reset counter on power-on
    reason = ResetReason::POWER_ON;
  } else {
    Serial.println(F("UNKNOWN"));
    reason = ResetReason::UNKNOWN;
  }

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
  // Delay that resets watchdog during wait
  // Prevents WDT timeout during long operations

  unsigned long start = millis();

  while ((millis() - start) < ms) {
    watchdogReset();
    delay(10);  // Short delay to avoid tight loop
  }
}
