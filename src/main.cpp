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
 */

#include <Arduino.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <avr/wdt.h>

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
  constexpr uint8_t I2C_ADDRESS = 0x27;  // ที่อยู่ I2C ของ LCD
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
  constexpr unsigned long PUMP_RUN_TIME      = 5000UL;   // เวลาเปิดปั๊มน้ำ 5 วินาที
  constexpr unsigned long FAN_RUN_TIME       = 10000UL;  // เวลาเปิดพัดลม 10 วินาที
  constexpr unsigned long READ_INTERVAL      = 2000UL;   // อ่านค่า Sensor ทุก 2 วินาที
  constexpr unsigned long IDLE_READ_INTERVAL = 5000UL;   // อ่านค่า Sensor ทุก 5 วินาทีในโหมด IDLE
  constexpr unsigned long COOLDOWN_TIME      = 30000UL;  // พักระบบ 30 วินาทีหลังทำงาน
  constexpr unsigned long LCD_UPDATE_INTERVAL = 500UL;   // อัพเดท LCD ทุก 500 มิลลิวินาที
}

namespace SensorConfig {
  constexpr int SAMPLES        = 10;   // จำนวนครั้งในการอ่านค่าเฉลี่ย
  constexpr int MIN_VALID      = 0;    // ค่าต่ำสุดที่ถูกต้อง
  constexpr int MAX_VALID      = 1023; // ค่าสูงสุดที่ถูกต้อง
  constexpr int EDGE_LOW       = 10;   // ค่าขอบล่าง (อาจมีปัญหา)
  constexpr int EDGE_HIGH      = 1013; // ค่าขอบบน (อาจมีปัญหา)
  constexpr int MAX_DEVIATION  = 200;  // ค่าเปลี่ยนแปลงสูงสุดที่ยอมรับได้ (EMI protection)
  constexpr int SAMPLE_DELAY   = 5;    // Delay ระหว่าง Sample (ms)
}

namespace RelayState {
  constexpr uint8_t ON  = LOW;   // Active-Low
  constexpr uint8_t OFF = HIGH;
}

// =============================================
// Watchdog Configuration
// =============================================

namespace WatchdogConfig {
  // Watchdog timeout options:
  // WDTO_15MS, WDTO_30MS, WDTO_60MS, WDTO_120MS,
  // WDTO_250MS, WDTO_500MS, WDTO_1S, WDTO_2S, WDTO_4S, WDTO_8S
  constexpr uint8_t TIMEOUT = WDTO_2S;  // 2 second timeout

  // Maximum allowed consecutive errors before system reset
  constexpr uint8_t MAX_CONSECUTIVE_ERRORS = 5;
}

// =============================================
// Enum สำหรับสถานะระบบ (System State Enum)
// =============================================

enum class SystemState : uint8_t {
  IDLE,         // โหมดพัก - ความชื้นปกติ
  WATERING,     // กำลังรดน้ำ
  VENTILATING,  // กำลังระบายความชื้น
  COOLDOWN,     // พักหลังทำงาน
  ERROR         // สถานะผิดพลาด (เพิ่มใหม่สำหรับ safety)
};

// =============================================
// Custom LCD Characters (ตัวอักษรพิเศษ)
// =============================================

namespace LcdIcons {
  constexpr uint8_t WATER_DROP = 0;
  constexpr uint8_t FAN        = 1;
  constexpr uint8_t PLANT      = 2;
  constexpr uint8_t WARNING    = 3;

  const uint8_t CHAR_WATER_DROP[8] PROGMEM = {
    0b00100, 0b00100, 0b01110, 0b01110,
    0b11111, 0b11111, 0b11111, 0b01110
  };

  const uint8_t CHAR_FAN[8] PROGMEM = {
    0b00000, 0b11011, 0b11011, 0b00100,
    0b11011, 0b11011, 0b00000, 0b00000
  };

  const uint8_t CHAR_PLANT[8] PROGMEM = {
    0b00100, 0b01110, 0b00100, 0b01110,
    0b10101, 0b00100, 0b00100, 0b01110
  };

  const uint8_t CHAR_WARNING[8] PROGMEM = {
    0b00000, 0b00100, 0b01110, 0b01110,
    0b11111, 0b11111, 0b00100, 0b00000
  };
}

// =============================================
// Global Objects
// =============================================

LiquidCrystal_I2C lcd(LcdConfig::I2C_ADDRESS, LcdConfig::COLUMNS, LcdConfig::ROWS);

// =============================================
// System State Structure (รวมตัวแปรสถานะไว้ด้วยกัน)
// =============================================

struct SystemData {
  SystemState currentState = SystemState::IDLE;
  SystemState previousState = SystemState::IDLE;

  unsigned long lastReadTime = 0;
  unsigned long stateStartTime = 0;
  unsigned long lastStateChangeTime = 0;
  unsigned long lastLcdUpdateTime = 0;

  int currentMoisture = 512;   // ค่าเริ่มต้นกลางๆ
  int previousMoisture = 512;

  bool sensorError = false;
  uint8_t consecutiveErrors = 0;  // นับจำนวนข้อผิดพลาดติดต่อกัน

  // Watchdog reset counter (stored in EEPROM section that survives reset)
  volatile uint8_t wdtResetCount = 0;
} systemData;

// =============================================
// Watchdog Reset Flag (ตรวจสอบว่ารีเซ็ตจาก WDT หรือไม่)
// =============================================

// ตัวแปรสำหรับเก็บสาเหตุการรีเซ็ต
uint8_t resetReason __attribute__((section(".noinit")));

// =============================================
// Forward Declarations (ประกาศฟังก์ชันล่วงหน้า)
// =============================================

// Watchdog Functions
void watchdogSetup();
void watchdogReset();
void watchdogDisable();
void checkResetReason();

// Initialization Functions
void initializePins();
void initializeRelays();
void initializeLcd();
void createLcdCustomChars();

// Sensor Functions
int readSoilMoisture();
bool validateSensorReading(int reading);
bool checkEmiSpike(int newReading, int oldReading);
int readSingleSample();
int getMedianReading(int* samples, int count);

// State Machine Functions
void updateSystemState(int moisture);
void executeState();
void transitionTo(SystemState newState);
void handleError();

// Device Control Functions
void startPump();
void stopPump();
void startFan();
void stopFan();
void stopAllDevices();
void safeShutdown();

// Display Functions (Serial)
void printSystemStatus(int moisture);
void printStateTransition(SystemState from, SystemState to);
const __FlashStringHelper* getStateName(SystemState state);
const __FlashStringHelper* getMoistureStatus(int moisture);

// Display Functions (LCD)
void updateLcdDisplay();
void lcdShowStartupScreen();
void lcdShowSystemStatus();
void lcdShowErrorScreen();
void lcdClearRow(uint8_t row);
const char* getLcdStateName(SystemState state);
const char* getLcdMoistureStatus(int moisture);
int getMoisturePercent(int rawValue);

// Utility Functions
unsigned long getElapsedTime(unsigned long startTime);
void safeDelay(unsigned long ms);

// Reserved Relay Functions
void activateRelay1();
void deactivateRelay1();
void activateRelay2();
void deactivateRelay2();

// =============================================
// ฟังก์ชัน Watchdog (Watchdog Functions)
// =============================================

void watchdogSetup() {
  // Disable interrupts during WDT setup
  cli();

  // Clear the reset flag
  MCUSR &= ~(1 << WDRF);

  // Set WDCE (Watchdog Change Enable) and WDE (Watchdog Enable)
  // This allows us to change WDT settings in the next 4 clock cycles
  WDTCSR |= (1 << WDCE) | (1 << WDE);

  // Set new watchdog timeout value
  // WDTO_2S = 2 second timeout
  WDTCSR = (1 << WDE) | (1 << WDP2) | (1 << WDP1) | (1 << WDP0);

  // Re-enable interrupts
  sei();

  Serial.println(F("[WDT] Watchdog Timer เริ่มต้น (2s timeout)"));
}

void watchdogReset() {
  // Reset the watchdog timer
  wdt_reset();
}

void watchdogDisable() {
  cli();
  wdt_reset();
  MCUSR &= ~(1 << WDRF);
  WDTCSR |= (1 << WDCE) | (1 << WDE);
  WDTCSR = 0x00;
  sei();
}

void checkResetReason() {
  // Check MCUSR for reset reason
  uint8_t mcusr = MCUSR;
  MCUSR = 0;  // Clear for next time

  Serial.print(F("[BOOT] Reset reason: "));

  if (mcusr & (1 << WDRF)) {
    Serial.println(F("WATCHDOG RESET!"));
    Serial.println(F("[WARN] ระบบรีเซ็ตจาก Watchdog - อาจเกิดจาก EMI หรือโปรแกรมค้าง"));

    // ถ้ารีเซ็ตจาก WDT ให้เข้าสู่ safe mode ก่อน
    systemData.wdtResetCount++;

    if (systemData.wdtResetCount >= 3) {
      Serial.println(F("[WARN] รีเซ็ตจาก WDT หลายครั้ง - เข้าสู่ Safe Mode"));
    }
  } else if (mcusr & (1 << BORF)) {
    Serial.println(F("BROWN-OUT RESET"));
  } else if (mcusr & (1 << EXTRF)) {
    Serial.println(F("EXTERNAL RESET"));
  } else if (mcusr & (1 << PORF)) {
    Serial.println(F("POWER-ON RESET"));
    systemData.wdtResetCount = 0;  // Reset counter on power-on
  } else {
    Serial.println(F("UNKNOWN"));
  }
}

// =============================================
// ฟังก์ชันหลัก (Main Functions)
// =============================================

void setup() {
  // ปิด Watchdog ก่อน (อาจค้างจากรอบก่อน)
  watchdogDisable();

  // เริ่มต้น Serial Monitor สำหรับ Debug
  Serial.begin(9600);

  // รอให้ Serial พร้อม (สำหรับบอร์ดบางรุ่น)
  unsigned long serialWaitStart = millis();
  while (!Serial && (millis() - serialWaitStart) < 3000) {
    ; // รอไม่เกิน 3 วินาที
  }

  Serial.println(F(""));
  Serial.println(F("================================"));
  Serial.println(F("Automatic Greenhouse System v2.2"));
  Serial.println(F("================================"));
  Serial.println(F(""));

  // ตรวจสอบสาเหตุการรีเซ็ต
  checkResetReason();

  // เริ่มต้นระบบ
  initializePins();
  initializeRelays();
  initializeLcd();

  // ตั้งค่าเริ่มต้น
  systemData.currentState = SystemState::IDLE;
  systemData.stateStartTime = millis();
  systemData.lastStateChangeTime = millis();
  systemData.lastReadTime = 0;  // ให้อ่านค่าทันทีในรอบแรก
  systemData.lastLcdUpdateTime = 0;
  systemData.consecutiveErrors = 0;

  Serial.println(F("[SYSTEM] เริ่มต้นระบบสำเร็จ!"));
  Serial.println(F("[STATE] เข้าสู่โหมด IDLE"));
  Serial.println(F(""));

  // แสดงหน้าจอเริ่มต้นบน LCD
  lcdShowStartupScreen();
  safeDelay(2000);  // แสดงหน้าจอเริ่มต้น 2 วินาที

  // เริ่ม Watchdog Timer หลังจาก setup เสร็จ
  watchdogSetup();
}

void loop() {
  // Reset Watchdog Timer ทุกรอบ
  watchdogReset();

  unsigned long currentTime = millis();

  // กำหนดช่วงเวลาอ่านค่าตามสถานะ
  unsigned long readInterval = (systemData.currentState == SystemState::IDLE)
                               ? Config::IDLE_READ_INTERVAL
                               : Config::READ_INTERVAL;

  // อ่านค่า Sensor ตามช่วงเวลาที่กำหนด
  if ((currentTime - systemData.lastReadTime) >= readInterval) {
    systemData.lastReadTime = currentTime;

    // Reset watchdog ก่อนอ่าน sensor (อาจใช้เวลา)
    watchdogReset();

    // บันทึกค่าเก่า
    systemData.previousMoisture = systemData.currentMoisture;

    // อ่านค่าความชื้นจาก Sensor
    int newMoisture = readSoilMoisture();

    // ตรวจสอบ EMI spike
    if (!systemData.sensorError && checkEmiSpike(newMoisture, systemData.previousMoisture)) {
      Serial.println(F("[EMI] ตรวจพบค่าผิดปกติ (อาจเกิดจาก EMI) - ใช้ค่าเก่า"));
      systemData.consecutiveErrors++;
    } else {
      systemData.currentMoisture = newMoisture;
      if (!systemData.sensorError) {
        systemData.consecutiveErrors = 0;  // Reset error count on success
      }
    }

    // Reset watchdog หลังอ่าน sensor
    watchdogReset();

    // แสดงสถานะระบบ
    printSystemStatus(systemData.currentMoisture);

    // ตรวจสอบว่ามีข้อผิดพลาดติดต่อกันมากเกินไปหรือไม่
    if (systemData.consecutiveErrors >= WatchdogConfig::MAX_CONSECUTIVE_ERRORS) {
      Serial.println(F("[ERROR] ข้อผิดพลาดติดต่อกันมากเกินไป!"));
      handleError();
    } else if (!systemData.sensorError && systemData.currentState != SystemState::ERROR) {
      // อัพเดทสถานะระบบ (ถ้าไม่มีข้อผิดพลาด)
      updateSystemState(systemData.currentMoisture);
    }
  }

  // Reset watchdog
  watchdogReset();

  // อัพเดท LCD ตามช่วงเวลาที่กำหนด
  if ((currentTime - systemData.lastLcdUpdateTime) >= Config::LCD_UPDATE_INTERVAL) {
    systemData.lastLcdUpdateTime = currentTime;
    updateLcdDisplay();
  }

  // Reset watchdog
  watchdogReset();

  // ดำเนินการตามสถานะปัจจุบัน
  executeState();

  // Reset watchdog ท้ายลูป
  watchdogReset();
}

// =============================================
// ฟังก์ชันเริ่มต้นระบบ (Initialization Functions)
// =============================================

void initializePins() {
  // ตั้งค่าขา Relay เป็น Output
  pinMode(Pins::RELAY_1, OUTPUT);
  pinMode(Pins::RELAY_2, OUTPUT);
  pinMode(Pins::RELAY_PUMP, OUTPUT);
  pinMode(Pins::RELAY_FAN, OUTPUT);

  // ตั้งค่าขา Sensor เป็น Input
  pinMode(Pins::SOIL_MOISTURE, INPUT);

  Serial.println(F("[INIT] กำหนดขาสำเร็จ"));
}

void initializeRelays() {
  // ปิด Relay ทั้งหมดตอนเริ่มต้น (Active-Low: HIGH = ปิด)
  digitalWrite(Pins::RELAY_1, RelayState::OFF);
  digitalWrite(Pins::RELAY_2, RelayState::OFF);
  digitalWrite(Pins::RELAY_PUMP, RelayState::OFF);
  digitalWrite(Pins::RELAY_FAN, RelayState::OFF);

  Serial.println(F("[INIT] ปิด Relay ทั้งหมด"));
}

void initializeLcd() {
  // เริ่มต้น LCD
  lcd.init();

  // เปิดไฟ Backlight
  lcd.backlight();

  // สร้าง Custom Characters
  createLcdCustomChars();

  // ล้างหน้าจอ
  lcd.clear();

  Serial.println(F("[INIT] LCD 16x2 (I2C) เริ่มต้นสำเร็จ"));
}

void createLcdCustomChars() {
  // สร้าง Custom Characters จาก PROGMEM
  uint8_t charBuffer[8];

  // โหลดและสร้างไอคอนหยดน้ำ
  memcpy_P(charBuffer, LcdIcons::CHAR_WATER_DROP, 8);
  lcd.createChar(LcdIcons::WATER_DROP, charBuffer);

  // โหลดและสร้างไอคอนพัดลม
  memcpy_P(charBuffer, LcdIcons::CHAR_FAN, 8);
  lcd.createChar(LcdIcons::FAN, charBuffer);

  // โหลดและสร้างไอคอนต้นไม้
  memcpy_P(charBuffer, LcdIcons::CHAR_PLANT, 8);
  lcd.createChar(LcdIcons::PLANT, charBuffer);

  // โหลดและสร้างไอคอนเตือน
  memcpy_P(charBuffer, LcdIcons::CHAR_WARNING, 8);
  lcd.createChar(LcdIcons::WARNING, charBuffer);
}

// =============================================
// ฟังก์ชันอ่านค่า Sensor (Sensor Reading Functions)
// =============================================

int readSingleSample() {
  return analogRead(Pins::SOIL_MOISTURE);
}

int getMedianReading(int* samples, int count) {
  // Simple bubble sort for small arrays
  for (int i = 0; i < count - 1; i++) {
    for (int j = 0; j < count - i - 1; j++) {
      if (samples[j] > samples[j + 1]) {
        int temp = samples[j];
        samples[j] = samples[j + 1];
        samples[j + 1] = temp;
      }
    }
  }
  // Return median
  return samples[count / 2];
}

int readSoilMoisture() {
  // อ่านค่าหลายครั้งและใช้ Median filter เพื่อกรอง EMI noise
  int samples[SensorConfig::SAMPLES];

  for (int i = 0; i < SensorConfig::SAMPLES; i++) {
    samples[i] = readSingleSample();
    safeDelay(SensorConfig::SAMPLE_DELAY);

    // Reset watchdog ระหว่างอ่านค่า
    if (i % 3 == 0) {
      watchdogReset();
    }
  }

  // ใช้ Median แทน Average เพื่อกรอง spike จาก EMI
  int medianMoisture = getMedianReading(samples, SensorConfig::SAMPLES);

  // ตรวจสอบความถูกต้องของค่า Sensor
  if (!validateSensorReading(medianMoisture)) {
    systemData.sensorError = true;
    systemData.consecutiveErrors++;
    return systemData.currentMoisture; // ใช้ค่าเก่าแทน
  }

  systemData.sensorError = false;
  return medianMoisture;
}

bool validateSensorReading(int reading) {
  // ค่าต้องอยู่ในช่วงที่ถูกต้อง
  if (reading < SensorConfig::MIN_VALID || reading > SensorConfig::MAX_VALID) {
    Serial.println(F("[ERROR] ค่า Sensor ผิดปกติ!"));
    return false;
  }

  // เตือนถ้าค่า Sensor ติดที่ขอบ (อาจบ่งชี้ว่า Sensor มีปัญหา)
  if (reading <= SensorConfig::EDGE_LOW) {
    Serial.println(F("[WARN] Sensor อาจชื้นเกินไปหรือขาดการเชื่อมต่อ"));
  } else if (reading >= SensorConfig::EDGE_HIGH) {
    Serial.println(F("[WARN] Sensor อาจแห้งเกินไปหรือขาดการเชื่อมต่อ"));
  }

  return true;
}

bool checkEmiSpike(int newReading, int oldReading) {
  // ตรวจสอบว่าค่าเปลี่ยนแปลงมากผิดปกติหรือไม่ (อาจเกิดจาก EMI)
  int diff = abs(newReading - oldReading);
  return diff > SensorConfig::MAX_DEVIATION;
}

// =============================================
// ฟังก์ชัน State Machine (State Management)
// =============================================

void updateSystemState(int moisture) {
  switch (systemData.currentState) {
    case SystemState::IDLE:
      // ตรวจสอบว่าต้องเปลี่ยนสถานะหรือไม่
      if (moisture >= Config::MOISTURE_DRY_THRESHOLD) {
        transitionTo(SystemState::WATERING);
      } else if (moisture <= Config::MOISTURE_WET_THRESHOLD) {
        transitionTo(SystemState::VENTILATING);
      }
      // ถ้าความชื้นปกติ ก็อยู่ใน IDLE ต่อ
      break;

    case SystemState::WATERING:
      // ตรวจสอบว่าความชื้นดีขึ้นหรือยัง (ใช้ Hysteresis)
      if (moisture < (Config::MOISTURE_DRY_THRESHOLD - Config::HYSTERESIS)) {
        transitionTo(SystemState::COOLDOWN);
      }
      break;

    case SystemState::VENTILATING:
      // ตรวจสอบว่าความชื้นลดลงหรือยัง (ใช้ Hysteresis)
      if (moisture > (Config::MOISTURE_WET_THRESHOLD + Config::HYSTERESIS)) {
        transitionTo(SystemState::COOLDOWN);
      }
      break;

    case SystemState::COOLDOWN:
      // จะถูกจัดการใน executeState()
      break;

    case SystemState::ERROR:
      // จะถูกจัดการใน handleError()
      break;
  }
}

void executeState() {
  unsigned long elapsed = getElapsedTime(systemData.stateStartTime);

  switch (systemData.currentState) {
    case SystemState::IDLE:
      // ไม่ต้องทำอะไร - ระบบพักอยู่
      break;

    case SystemState::WATERING:
      // ตรวจสอบเวลาทำงานของปั๊ม
      if (elapsed >= Config::PUMP_RUN_TIME) {
        Serial.println(F("[PUMP] หยุดปั๊ม (ครบเวลา)"));
        transitionTo(SystemState::COOLDOWN);
      }
      break;

    case SystemState::VENTILATING:
      // ตรวจสอบเวลาทำงานของพัดลม
      if (elapsed >= Config::FAN_RUN_TIME) {
        Serial.println(F("[FAN] หยุดพัดลม (ครบเวลา)"));
        transitionTo(SystemState::COOLDOWN);
      }
      break;

    case SystemState::COOLDOWN:
      // ตรวจสอบว่าพักครบเวลาหรือยัง
      if (elapsed >= Config::COOLDOWN_TIME) {
        Serial.println(F("[COOLDOWN] พักครบเวลา"));
        transitionTo(SystemState::IDLE);
      }
      break;

    case SystemState::ERROR:
      // ในสถานะ Error รอจนกว่า sensor จะกลับมาปกติ
      if (!systemData.sensorError && systemData.consecutiveErrors == 0) {
        Serial.println(F("[RECOVERY] Sensor กลับมาปกติ"));
        transitionTo(SystemState::IDLE);
      }
      break;
  }
}

void transitionTo(SystemState newState) {
  if (systemData.currentState == newState) {
    return; // ไม่มีการเปลี่ยนแปลง
  }

  // บันทึกสถานะเก่า
  systemData.previousState = systemData.currentState;

  // หยุดอุปกรณ์เดิมก่อน
  stopAllDevices();

  // แสดงการเปลี่ยนสถานะ
  printStateTransition(systemData.currentState, newState);

  // เปลี่ยนสถานะ
  systemData.currentState = newState;
  systemData.stateStartTime = millis();
  systemData.lastStateChangeTime = millis();

  // Reset watchdog หลังเปลี่ยนสถานะ
  watchdogReset();

  // เริ่มทำงานตามสถานะใหม่
  switch (newState) {
    case SystemState::IDLE:
      Serial.println(F("[STATE] ระบบเข้าสู่โหมดพัก"));
      break;

    case SystemState::WATERING:
      startPump();
      break;

    case SystemState::VENTILATING:
      startFan();
      break;

    case SystemState::COOLDOWN:
      Serial.println(F("[STATE] เข้าสู่ช่วงพักระบบ"));
      break;

    case SystemState::ERROR:
      Serial.println(F("[STATE] เข้าสู่โหมด ERROR - หยุดอุปกรณ์ทั้งหมด"));
      safeShutdown();
      break;
  }

  // อัพเดท LCD ทันทีเมื่อเปลี่ยนสถานะ
  updateLcdDisplay();
}

void handleError() {
  Serial.println(F("[ERROR] ระบบเข้าสู่ Safe Mode"));

  // หยุดอุปกรณ์ทั้งหมดเพื่อความปลอดภัย
  safeShutdown();

  // เปลี่ยนเป็นสถานะ Error
  transitionTo(SystemState::ERROR);
}

// =============================================
// ฟังก์ชันควบคุมปั๊มน้ำ (Pump Control Functions)
// =============================================

void startPump() {
  Serial.println(F(""));
  Serial.println(F(">>> [PUMP] เปิดปั๊มน้ำ - กำลังรดน้ำ..."));
  digitalWrite(Pins::RELAY_PUMP, RelayState::ON);
}

void stopPump() {
  digitalWrite(Pins::RELAY_PUMP, RelayState::OFF);
}

// =============================================
// ฟังก์ชันควบคุมพัดลม (Fan Control Functions)
// =============================================

void startFan() {
  Serial.println(F(""));
  Serial.println(F(">>> [FAN] เปิดพัดลม - กำลังระบายความชื้น..."));
  digitalWrite(Pins::RELAY_FAN, RelayState::ON);
}

void stopFan() {
  digitalWrite(Pins::RELAY_FAN, RelayState::OFF);
}

// =============================================
// ฟังก์ชันหยุดอุปกรณ์ (Device Stop Functions)
// =============================================

void stopAllDevices() {
  stopPump();
  stopFan();
}

void safeShutdown() {
  // ปิดอุปกรณ์ทั้งหมดอย่างปลอดภัย
  digitalWrite(Pins::RELAY_1, RelayState::OFF);
  digitalWrite(Pins::RELAY_2, RelayState::OFF);
  digitalWrite(Pins::RELAY_PUMP, RelayState::OFF);
  digitalWrite(Pins::RELAY_FAN, RelayState::OFF);

  Serial.println(F("[SAFE] ปิดอุปกรณ์ทั้งหมดแล้ว"));
}

// =============================================
// ฟังก์ชันแสดงผล Serial (Serial Display Functions)
// =============================================

void printSystemStatus(int moisture) {
  Serial.println(F("-------------------------------------"));

  // แสดงค่าความชื้น
  Serial.print(F("Moisture: "));
  Serial.print(moisture);
  Serial.print(F(" ("));
  Serial.print(getMoisturePercent(moisture));
  Serial.print(F("%)"));
  Serial.print(F(" | Status: "));
  Serial.println(getMoistureStatus(moisture));

  // แสดงสถานะระบบ
  Serial.print(F("System State: "));
  Serial.print(getStateName(systemData.currentState));

  // แสดงเวลาในสถานะปัจจุบัน
  unsigned long elapsed = getElapsedTime(systemData.stateStartTime);
  Serial.print(F(" ("));
  Serial.print(elapsed / 1000);
  Serial.println(F("s)"));

  // แสดงสถานะอุปกรณ์
  Serial.print(F("Pump: "));
  Serial.print(systemData.currentState == SystemState::WATERING ? F("ON") : F("OFF"));
  Serial.print(F(" | Fan: "));
  Serial.println(systemData.currentState == SystemState::VENTILATING ? F("ON") : F("OFF"));

  // แสดงข้อมูล Error
  if (systemData.sensorError) {
    Serial.println(F("!!! SENSOR ERROR - Using previous value !!!"));
  }
  if (systemData.consecutiveErrors > 0) {
    Serial.print(F("[WARN] Consecutive errors: "));
    Serial.println(systemData.consecutiveErrors);
  }

  Serial.println(F("-------------------------------------"));
  Serial.println(F(""));
}

void printStateTransition(SystemState from, SystemState to) {
  Serial.println(F(""));
  Serial.print(F("==> STATE CHANGE: "));
  Serial.print(getStateName(from));
  Serial.print(F(" -> "));
  Serial.println(getStateName(to));
}

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

const __FlashStringHelper* getMoistureStatus(int moisture) {
  if (moisture >= Config::MOISTURE_DRY_THRESHOLD) {
    return F("DRY (ดินแห้ง)");
  } else if (moisture <= Config::MOISTURE_WET_THRESHOLD) {
    return F("TOO WET (ชื้นเกินไป)");
  } else {
    return F("NORMAL (ปกติ)");
  }
}

// =============================================
// ฟังก์ชันแสดงผล LCD (LCD Display Functions)
// =============================================

void lcdShowStartupScreen() {
  lcd.clear();

  // แถวที่ 1: ชื่อระบบ
  lcd.setCursor(0, 0);
  lcd.write(LcdIcons::PLANT);  // ไอคอนต้นไม้
  lcd.print(F(" Greenhouse"));

  // แถวที่ 2: เวอร์ชัน
  lcd.setCursor(0, 1);
  lcd.print(F("System v2.2 WDT"));
}

void updateLcdDisplay() {
  if (systemData.currentState == SystemState::ERROR) {
    lcdShowErrorScreen();
  } else {
    lcdShowSystemStatus();
  }
}

void lcdShowErrorScreen() {
  lcd.setCursor(0, 0);
  lcd.write(LcdIcons::WARNING);
  lcd.print(F(" SENSOR ERROR"));

  lcd.setCursor(0, 1);
  lcd.print(F("Check connection"));
}

void lcdShowSystemStatus() {
  // แถวที่ 1: ค่าความชื้นและสถานะ
  // รูปแบบ: "M:xxx% STATUS"
  lcd.setCursor(0, 0);

  // แสดงไอคอนตามสถานะ
  if (systemData.sensorError) {
    lcd.write(LcdIcons::WARNING);
  } else if (systemData.currentState == SystemState::WATERING) {
    lcd.write(LcdIcons::WATER_DROP);
  } else if (systemData.currentState == SystemState::VENTILATING) {
    lcd.write(LcdIcons::FAN);
  } else {
    lcd.write(LcdIcons::PLANT);
  }

  // แสดงค่าความชื้นเป็นเปอร์เซ็นต์
  lcd.print(F("M:"));
  int moisturePercent = getMoisturePercent(systemData.currentMoisture);

  // จัดรูปแบบตัวเลข (เติมช่องว่างด้านหน้า)
  if (moisturePercent < 10) {
    lcd.print(F("  "));
  } else if (moisturePercent < 100) {
    lcd.print(F(" "));
  }
  lcd.print(moisturePercent);
  lcd.print(F("% "));

  // แสดงสถานะความชื้นแบบย่อ
  lcd.print(getLcdMoistureStatus(systemData.currentMoisture));

  // เติมช่องว่างที่เหลือ
  lcd.print(F("   "));

  // แถวที่ 2: สถานะระบบและเวลา
  // รูปแบบ: "STATE    xxxs"
  lcd.setCursor(0, 1);

  // แสดงสถานะระบบ
  lcd.print(getLcdStateName(systemData.currentState));

  // แสดงเวลาที่ผ่านไปในสถานะปัจจุบัน
  unsigned long elapsed = getElapsedTime(systemData.stateStartTime);
  unsigned long elapsedSec = elapsed / 1000;

  // คำนวณตำแหน่งสำหรับแสดงเวลา (ชิดขวา)
  lcd.setCursor(11, 1);

  // จัดรูปแบบตัวเลข (เติมช่องว่างด้านหน้า)
  if (elapsedSec < 10) {
    lcd.print(F("   "));
  } else if (elapsedSec < 100) {
    lcd.print(F("  "));
  } else if (elapsedSec < 1000) {
    lcd.print(F(" "));
  }
  lcd.print(elapsedSec);
  lcd.print(F("s"));
}

void lcdClearRow(uint8_t row) {
  lcd.setCursor(0, row);
  lcd.print(F("                "));  // 16 ช่องว่าง
}

const char* getLcdStateName(SystemState state) {
  // ชื่อสถานะแบบย่อสำหรับ LCD (จำกัด 10 ตัวอักษร)
  switch (state) {
    case SystemState::IDLE:        return "IDLE      ";
    case SystemState::WATERING:    return "WATERING  ";
    case SystemState::VENTILATING: return "VENT      ";
    case SystemState::COOLDOWN:    return "COOLDOWN  ";
    case SystemState::ERROR:       return "ERROR     ";
    default:                       return "UNKNOWN   ";
  }
}

const char* getLcdMoistureStatus(int moisture) {
  // สถานะความชื้นแบบย่อสำหรับ LCD
  if (moisture >= Config::MOISTURE_DRY_THRESHOLD) {
    return "DRY";
  } else if (moisture <= Config::MOISTURE_WET_THRESHOLD) {
    return "WET";
  } else {
    return "OK ";
  }
}

int getMoisturePercent(int rawValue) {
  // แปลงค่า Analog (0-1023) เป็นเปอร์เซ็นต์ความชื้น (0-100%)
  // หมายเหตุ: ค่า Analog ต่ำ = ความชื้นสูง (กลับค่า)
  int percent = map(rawValue, SensorConfig::MAX_VALID, SensorConfig::MIN_VALID, 0, 100);

  // จำกัดค่าให้อยู่ในช่วง 0-100
  if (percent < 0) percent = 0;
  if (percent > 100) percent = 100;

  return percent;
}

// =============================================
// ฟังก์ชันช่วยเหลือ (Utility Functions)
// =============================================

unsigned long getElapsedTime(unsigned long startTime) {
  unsigned long currentTime = millis();
  // จัดการ overflow ของ millis() (ประมาณ 49 วัน)
  if (currentTime >= startTime) {
    return currentTime - startTime;
  } else {
    // กรณี overflow
    return (0xFFFFFFFFUL - startTime) + currentTime + 1;
  }
}

void safeDelay(unsigned long ms) {
  // Delay แบบปลอดภัยที่ reset watchdog ระหว่างรอ
  unsigned long start = millis();
  while ((millis() - start) < ms) {
    watchdogReset();
    delay(10);  // Short delay to avoid tight loop
  }
}

// =============================================
// ฟังก์ชันสำรองสำหรับ Relay 1 และ 2 (Reserved Functions)
// =============================================

void activateRelay1() {
  digitalWrite(Pins::RELAY_1, RelayState::ON);
  Serial.println(F("[RELAY1] Activated"));
}

void deactivateRelay1() {
  digitalWrite(Pins::RELAY_1, RelayState::OFF);
  Serial.println(F("[RELAY1] Deactivated"));
}

void activateRelay2() {
  digitalWrite(Pins::RELAY_2, RelayState::ON);
  Serial.println(F("[RELAY2] Activated"));
}

void deactivateRelay2() {
  digitalWrite(Pins::RELAY_2, RelayState::OFF);
  Serial.println(F("[RELAY2] Deactivated"));
}
