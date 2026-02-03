/*
 * Config.h - Configuration Constants for Automatic Greenhouse System
 *
 * รวมค่าคงที่ทั้งหมดสำหรับการตั้งค่าระบบ
 * - Pin definitions
 * - Timing configurations
 * - Threshold values
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
// การกำหนดขา (Pin Definitions)
// =============================================

namespace Pins {
  // Sensor วัดความชื้นในดิน
  constexpr uint8_t SOIL_MOISTURE = A0;  // ขา Analog สำหรับอ่านค่าความชื้น

  // Relay 4-Channel (Active-Low: LOW = เปิด, HIGH = ปิด)
  constexpr uint8_t RELAY_1    = 2;   // IN1 - รอต่อใช้งาน
  constexpr uint8_t RELAY_2    = 3;   // IN2 - รอต่อใช้งาน
  constexpr uint8_t RELAY_PUMP = 4;   // IN3 - ควบคุมปั๊มน้ำ
  constexpr uint8_t RELAY_FAN  = 5;   // IN4 - ควบคุมพัดลม
}

// =============================================
// การกำหนดค่า LCD (LCD Configuration)
// =============================================

namespace LcdConfig {
  constexpr uint8_t I2C_ADDRESS = 0x27;  // ที่อยู่ I2C ของ LCD (0x27 หรือ 0x3F)
  constexpr uint8_t COLUMNS     = 16;    // จำนวนคอลัมน์ของ LCD
  constexpr uint8_t ROWS        = 2;     // จำนวนแถวของ LCD
}

// =============================================
// ค่าคงที่สำหรับการตั้งค่า (Configuration Constants)
// =============================================

namespace Config {
  // ค่าความชื้น (0-1023 จาก Analog Read)
  // หมายเหตุ: ค่าต่ำ = ความชื้นสูง, ค่าสูง = ความชื้นต่ำ (สำหรับ Sensor ส่วนใหญ่)
  constexpr int MOISTURE_DRY_THRESHOLD = 700;   // ค่าขีดจำกัดดินแห้ง (ต้องรดน้ำ)
  constexpr int MOISTURE_WET_THRESHOLD = 300;   // ค่าขีดจำกัดดินชื้นเกินไป (ต้องเปิดพัดลม)

  // ค่า Hysteresis เพื่อป้องกันการสั่นสะเทือนของระบบ
  constexpr int HYSTERESIS = 50;

  // ระยะเวลาในการทำงาน (มิลลิวินาที)
  constexpr unsigned long PUMP_RUN_TIME       = 5000UL;   // เวลาเปิดปั๊มน้ำ 5 วินาที
  constexpr unsigned long FAN_RUN_TIME        = 10000UL;  // เวลาเปิดพัดลม 10 วินาที
  constexpr unsigned long READ_INTERVAL       = 2000UL;   // อ่านค่า Sensor ทุก 2 วินาที
  constexpr unsigned long IDLE_READ_INTERVAL  = 5000UL;   // อ่านค่า Sensor ทุก 5 วินาทีในโหมด IDLE
  constexpr unsigned long COOLDOWN_TIME       = 30000UL;  // พักระบบ 30 วินาทีหลังทำงาน
  constexpr unsigned long LCD_UPDATE_INTERVAL = 500UL;    // อัพเดท LCD ทุก 500 มิลลิวินาที
}

// =============================================
// ค่าสำหรับการอ่าน Sensor (Sensor Configuration)
// =============================================

namespace SensorConfig {
  constexpr int SAMPLES       = 10;    // จำนวนครั้งในการอ่านค่าเฉลี่ย
  constexpr int MIN_VALID     = 0;     // ค่าต่ำสุดที่ถูกต้อง
  constexpr int MAX_VALID     = 1023;  // ค่าสูงสุดที่ถูกต้อง
  constexpr int EDGE_LOW      = 10;    // ค่าขอบล่าง (อาจมีปัญหา)
  constexpr int EDGE_HIGH     = 1013;  // ค่าขอบบน (อาจมีปัญหา)
  constexpr int MAX_DEVIATION = 200;   // ค่าเปลี่ยนแปลงสูงสุดที่ยอมรับได้ (EMI protection)
  constexpr int SAMPLE_DELAY  = 5;     // Delay ระหว่าง Sample (ms)
}

// =============================================
// สถานะ Relay (Relay State)
// =============================================

namespace RelayState {
  constexpr uint8_t ON  = LOW;   // Active-Low
  constexpr uint8_t OFF = HIGH;
}

// =============================================
// Watchdog Configuration
// =============================================

namespace WatchdogConfig {
  // Watchdog timeout: WDTO_15MS, WDTO_30MS, WDTO_60MS, WDTO_120MS,
  // WDTO_250MS, WDTO_500MS, WDTO_1S, WDTO_2S, WDTO_4S, WDTO_8S
  constexpr uint8_t TIMEOUT = WDTO_2S;  // 2 second timeout

  // Maximum allowed consecutive errors before entering error state
  constexpr uint8_t MAX_CONSECUTIVE_ERRORS = 5;
}

#endif // CONFIG_H
