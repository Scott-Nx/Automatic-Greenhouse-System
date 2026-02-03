/*
 * Config.h - Configuration Constants for Automatic Greenhouse System
 *
 * This file contains all configuration constants for the system.
 * - Pin definitions
 * - Timing configurations
 * - Threshold values
 * - Watchdog settings
 *
 * Version: 2.2
 */

#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>
#include <avr/wdt.h>

// =============================================
// Version Information
// =============================================

#define SYSTEM_VERSION "2.2"
#define SYSTEM_NAME "Greenhouse System"

// =============================================
// Pin Definitions
// =============================================

namespace Pins {
  // Soil moisture sensor analog input pin
  constexpr uint8_t SOIL_MOISTURE = A0;  // ขา Analog สำหรับอ่านค่าความชื้น (เปลี่ยนได้ตามการต่อวงจร)

  // 4-Channel Relay pins (Active-Low: LOW = ON, HIGH = OFF)
  constexpr uint8_t RELAY_1    = 2;   // IN1 - สำรองไว้ใช้งาน
  constexpr uint8_t RELAY_2    = 3;   // IN2 - สำรองไว้ใช้งาน
  constexpr uint8_t RELAY_PUMP = 4;   // IN3 - ขาควบคุมปั๊มน้ำ (เปลี่ยนได้)
  constexpr uint8_t RELAY_FAN  = 5;   // IN4 - ขาควบคุมพัดลม (เปลี่ยนได้)
}

// =============================================
// LCD Configuration
// =============================================

namespace LcdConfig {
  constexpr uint8_t I2C_ADDRESS = 0x27;  // ที่อยู่ I2C ของ LCD (ปกติ 0x27 หรือ 0x3F - ใช้ I2C Scanner ตรวจสอบ)
  constexpr uint8_t COLUMNS     = 16;    // จำนวนคอลัมน์ของ LCD
  constexpr uint8_t ROWS        = 2;     // จำนวนแถวของ LCD
}

// =============================================
// User-Configurable Threshold Values
// =============================================

namespace Config {
  // Soil moisture thresholds (0-1023 from analogRead)
  // NOTE: Lower value = higher moisture, Higher value = lower moisture (for most sensors)
  constexpr int MOISTURE_DRY_THRESHOLD = 700;   // ค่าความชื้นที่ถือว่าดินแห้ง (เปิดปั๊มน้ำ) - ปรับได้ 0-1023
  constexpr int MOISTURE_WET_THRESHOLD = 300;   // ค่าความชื้นที่ถือว่าดินเปียกเกินไป (เปิดพัดลม) - ปรับได้ 0-1023

  // Hysteresis value to prevent system oscillation
  constexpr int HYSTERESIS = 50;  // ค่า Hysteresis ป้องกันระบบสั่น - ปรับได้ตามความเหมาะสม

  // Timing configurations (in milliseconds)
  constexpr unsigned long PUMP_RUN_TIME       = 5000UL;   // เวลาเปิดปั๊มน้ำ (มิลลิวินาที) - ปรับได้
  constexpr unsigned long FAN_RUN_TIME        = 10000UL;  // เวลาเปิดพัดลม (มิลลิวินาที) - ปรับได้
  constexpr unsigned long READ_INTERVAL       = 2000UL;   // ช่วงเวลาอ่านค่า Sensor ปกติ (มิลลิวินาที)
  constexpr unsigned long IDLE_READ_INTERVAL  = 5000UL;   // ช่วงเวลาอ่านค่า Sensor ในโหมด IDLE (มิลลิวินาที)
  constexpr unsigned long COOLDOWN_TIME       = 30000UL;  // เวลาพักระบบหลังทำงาน (มิลลิวินาที) - ปรับได้
  constexpr unsigned long LCD_UPDATE_INTERVAL = 500UL;    // ช่วงเวลาอัพเดท LCD (มิลลิวินาที)
}

// =============================================
// Sensor Configuration
// =============================================

namespace SensorConfig {
  // Number of samples for median filter (EMI noise reduction)
  constexpr int SAMPLES       = 10;    // จำนวนครั้งในการอ่านค่าเฉลี่ย - ยิ่งมากยิ่งแม่นยำแต่ช้าลง

  // Valid range for sensor readings
  constexpr int MIN_VALID     = 0;     // Minimum valid ADC value
  constexpr int MAX_VALID     = 1023;  // Maximum valid ADC value

  // Edge values that may indicate sensor issues
  constexpr int EDGE_LOW      = 10;    // Values below this may indicate short circuit
  constexpr int EDGE_HIGH     = 1013;  // Values above this may indicate open circuit

  // EMI spike detection threshold
  constexpr int MAX_DEVIATION = 200;   // ค่าเปลี่ยนแปลงสูงสุดที่ยอมรับ (ป้องกัน EMI) - ปรับได้ 100-300

  // Delay between samples during median filter reading
  constexpr int SAMPLE_DELAY  = 5;     // Delay between samples in milliseconds
}

// =============================================
// Relay State Constants
// =============================================

namespace RelayState {
  // Active-Low relay module: LOW turns ON, HIGH turns OFF
  constexpr uint8_t ON  = LOW;
  constexpr uint8_t OFF = HIGH;
}

// =============================================
// Watchdog Timer Configuration
// =============================================

namespace WatchdogConfig {
  /*
   * Watchdog timeout options (from avr/wdt.h):
   *   WDTO_15MS   - 15 milliseconds
   *   WDTO_30MS   - 30 milliseconds
   *   WDTO_60MS   - 60 milliseconds
   *   WDTO_120MS  - 120 milliseconds
   *   WDTO_250MS  - 250 milliseconds
   *   WDTO_500MS  - 500 milliseconds
   *   WDTO_1S     - 1 second
   *   WDTO_2S     - 2 seconds (default)
   *   WDTO_4S     - 4 seconds
   *   WDTO_8S     - 8 seconds (maximum)
   *
   * Choose based on your needs:
   * - Shorter timeout = faster recovery from hang, but risk of false resets
   * - Longer timeout = more tolerance for slow operations, but slower recovery
   */
  constexpr uint8_t TIMEOUT = WDTO_2S;  // เวลา Timeout ของ Watchdog - แนะนำ WDTO_2S หรือ WDTO_4S

  // Maximum consecutive sensor errors before entering ERROR state
  constexpr uint8_t MAX_CONSECUTIVE_ERRORS = 5;  // จำนวนข้อผิดพลาดติดต่อกันสูงสุด - ปรับได้ 3-10
}

#endif // CONFIG_H
