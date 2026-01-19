/*
 * ระบบโรงเรือนอัตโนมัติ (Automatic Greenhouse System)
 * สำหรับบอร์ด Arduino Uno
 *
 * ระบบควบคุมความชื้นในดินอัตโนมัติ:
 * - เมื่อความชื้นปกติ: โหมด IDLE (พักระบบ)
 * - เมื่อความชื้นต่ำ: เปิดปั๊มน้ำเพื่อรดน้ำ
 * - เมื่อความชื้นสูง: เปิดพัดลมเพื่อดูดความชื้นออก
 */

#include <Arduino.h>

// =============================================
// การกำหนดขา (Pin Definitions)
// =============================================

// Sensor วัดความชื้นในดิน
constexpr uint8_t SOIL_MOISTURE_PIN = A0;  // ขา Analog สำหรับอ่านค่าความชื้น

// Relay 4-Channel (Active-Low: LOW = เปิด, HIGH = ปิด)
constexpr uint8_t RELAY_1_PIN    = 2;   // IN1 - รอต่อใช้งาน
constexpr uint8_t RELAY_2_PIN    = 3;   // IN2 - รอต่อใช้งาน
constexpr uint8_t RELAY_PUMP_PIN = 4;   // IN3 - ควบคุมปั๊มน้ำ
constexpr uint8_t RELAY_FAN_PIN  = 5;   // IN4 - ควบคุมพัดลม

// =============================================
// ค่าคงที่สำหรับการตั้งค่า (Configuration Constants)
// =============================================

// ค่าความชื้น (0-1023 จาก Analog Read)
// หมายเหตุ: ค่าต่ำ = ความชื้นสูง, ค่าสูง = ความชื้นต่ำ (สำหรับ Sensor ส่วนใหญ่)
constexpr int MOISTURE_DRY_THRESHOLD = 700;   // ค่าขีดจำกัดดินแห้ง (ต้องรดน้ำ)
constexpr int MOISTURE_WET_THRESHOLD = 300;   // ค่าขีดจำกัดดินชื้นเกินไป (ต้องเปิดพัดลม)

// ค่า Hysteresis เพื่อป้องกันการสั่นสะเทือนของระบบ
constexpr int HYSTERESIS = 50;

// ระยะเวลาในการทำงาน (มิลลิวินาที)
constexpr unsigned long PUMP_RUN_TIME     = 5000UL;   // เวลาเปิดปั๊มน้ำ 5 วินาที
constexpr unsigned long FAN_RUN_TIME      = 10000UL;  // เวลาเปิดพัดลม 10 วินาที
constexpr unsigned long READ_INTERVAL     = 2000UL;   // อ่านค่า Sensor ทุก 2 วินาที
constexpr unsigned long IDLE_READ_INTERVAL = 5000UL; // อ่านค่า Sensor ทุก 5 วินาทีในโหมด IDLE
constexpr unsigned long COOLDOWN_TIME     = 30000UL;  // พักระบบ 30 วินาทีหลังทำงาน

// ค่าสำหรับการอ่าน Sensor
constexpr int SENSOR_SAMPLES     = 10;   // จำนวนครั้งในการอ่านค่าเฉลี่ย
constexpr int SENSOR_MIN_VALID   = 0;    // ค่าต่ำสุดที่ถูกต้อง
constexpr int SENSOR_MAX_VALID   = 1023; // ค่าสูงสุดที่ถูกต้อง
constexpr int SENSOR_EDGE_LOW    = 10;   // ค่าขอบล่าง (อาจมีปัญหา)
constexpr int SENSOR_EDGE_HIGH   = 1013; // ค่าขอบบน (อาจมีปัญหา)

// สถานะ Relay (Active-Low)
constexpr uint8_t RELAY_ON  = LOW;
constexpr uint8_t RELAY_OFF = HIGH;

// =============================================
// Enum สำหรับสถานะระบบ (System State Enum)
// =============================================

enum class SystemState : uint8_t {
  IDLE,       // โหมดพัก - ความชื้นปกติ
  WATERING,   // กำลังรดน้ำ
  VENTILATING,// กำลังระบายความชื้น
  COOLDOWN    // พักหลังทำงาน
};

// =============================================
// ตัวแปรสถานะ (State Variables)
// =============================================

SystemState currentState = SystemState::IDLE;
SystemState previousState = SystemState::IDLE;

unsigned long lastReadTime = 0;
unsigned long stateStartTime = 0;
unsigned long lastStateChangeTime = 0;

int currentMoisture = 512;  // ค่าเริ่มต้นกลางๆ
int previousMoisture = 512;

bool sensorError = false;

// =============================================
// Forward Declarations (ประกาศฟังก์ชันล่วงหน้า)
// =============================================

void initializePins();
void initializeRelays();
int readSoilMoisture();
bool validateSensorReading(int reading);
void updateSystemState(int moisture);
void executeState();
void transitionTo(SystemState newState);
void startPump();
void stopPump();
void startFan();
void stopFan();
void stopAllDevices();
void printSystemStatus(int moisture);
void printStateTransition(SystemState from, SystemState to);
const __FlashStringHelper* getStateName(SystemState state);
const __FlashStringHelper* getMoistureStatus(int moisture);
unsigned long getElapsedTime(unsigned long startTime);
void activateRelay1();
void deactivateRelay1();
void activateRelay2();
void deactivateRelay2();

// =============================================
// ฟังก์ชันหลัก (Main Functions)
// =============================================

void setup() {
  // เริ่มต้น Serial Monitor สำหรับ Debug
  Serial.begin(9600);

  // รอให้ Serial พร้อม (สำหรับบอร์ดบางรุ่น)
  while (!Serial && millis() < 3000) {
    ; // รอไม่เกิน 3 วินาที
  }

  Serial.println(F(""));
  Serial.println(F("====================================="));
  Serial.println(F("  Automatic Greenhouse System v2.0"));
  Serial.println(F("  ระบบโรงเรือนอัตโนมัติ"));
  Serial.println(F("====================================="));
  Serial.println(F(""));

  // เริ่มต้นระบบ
  initializePins();
  initializeRelays();

  // ตั้งค่าเริ่มต้น
  currentState = SystemState::IDLE;
  stateStartTime = millis();
  lastStateChangeTime = millis();
  lastReadTime = 0;  // ให้อ่านค่าทันทีในรอบแรก

  Serial.println(F("[SYSTEM] เริ่มต้นระบบสำเร็จ!"));
  Serial.println(F("[STATE] เข้าสู่โหมด IDLE"));
  Serial.println(F(""));
}

void loop() {
  unsigned long currentTime = millis();

  // กำหนดช่วงเวลาอ่านค่าตามสถานะ
  unsigned long readInterval = (currentState == SystemState::IDLE) ? IDLE_READ_INTERVAL : READ_INTERVAL;

  // อ่านค่า Sensor ตามช่วงเวลาที่กำหนด
  if (currentTime - lastReadTime >= readInterval) {
    lastReadTime = currentTime;

    // บันทึกค่าเก่า
    previousMoisture = currentMoisture;

    // อ่านค่าความชื้นจาก Sensor
    currentMoisture = readSoilMoisture();

    // แสดงสถานะระบบ
    printSystemStatus(currentMoisture);

    // อัพเดทสถานะระบบ (ถ้าไม่มีข้อผิดพลาด)
    if (!sensorError) {
      updateSystemState(currentMoisture);
    }
  }

  // ดำเนินการตามสถานะปัจจุบัน
  executeState();
}

// =============================================
// ฟังก์ชันเริ่มต้นระบบ (Initialization Functions)
// =============================================

void initializePins() {
  // ตั้งค่าขา Relay เป็น Output
  pinMode(RELAY_1_PIN, OUTPUT);
  pinMode(RELAY_2_PIN, OUTPUT);
  pinMode(RELAY_PUMP_PIN, OUTPUT);
  pinMode(RELAY_FAN_PIN, OUTPUT);

  // ตั้งค่าขา Sensor เป็น Input (ไม่จำเป็นสำหรับ Analog แต่ชัดเจนดี)
  pinMode(SOIL_MOISTURE_PIN, INPUT);

  Serial.println(F("[INIT] กำหนดขาสำเร็จ"));
}

void initializeRelays() {
  // ปิด Relay ทั้งหมดตอนเริ่มต้น (Active-Low: HIGH = ปิด)
  digitalWrite(RELAY_1_PIN, RELAY_OFF);
  digitalWrite(RELAY_2_PIN, RELAY_OFF);
  digitalWrite(RELAY_PUMP_PIN, RELAY_OFF);
  digitalWrite(RELAY_FAN_PIN, RELAY_OFF);

  Serial.println(F("[INIT] ปิด Relay ทั้งหมด"));
}

// =============================================
// ฟังก์ชันอ่านค่า Sensor (Sensor Reading Functions)
// =============================================

int readSoilMoisture() {
  // อ่านค่าหลายครั้งแล้วหาค่าเฉลี่ย เพื่อลด Noise
  long sum = 0;  // ใช้ long เพื่อป้องกัน overflow

  for (int i = 0; i < SENSOR_SAMPLES; i++) {
    sum += analogRead(SOIL_MOISTURE_PIN);
    delay(5);  // ลดเวลา delay ลง
  }

  int avgMoisture = (int)(sum / SENSOR_SAMPLES);

  // ตรวจสอบความถูกต้องของค่า Sensor
  if (!validateSensorReading(avgMoisture)) {
    sensorError = true;
    return currentMoisture; // ใช้ค่าเก่าแทน
  }

  sensorError = false;
  return avgMoisture;
}

bool validateSensorReading(int reading) {
  // ค่าต้องอยู่ในช่วงที่ถูกต้อง
  if (reading < SENSOR_MIN_VALID || reading > SENSOR_MAX_VALID) {
    Serial.println(F("[ERROR] ค่า Sensor ผิดปกติ!"));
    return false;
  }

  // เตือนถ้าค่า Sensor ติดที่ขอบ (อาจบ่งชี้ว่า Sensor มีปัญหา)
  if (reading <= SENSOR_EDGE_LOW) {
    Serial.println(F("[WARN] Sensor อาจชื้นเกินไปหรือขาดการเชื่อมต่อ"));
  } else if (reading >= SENSOR_EDGE_HIGH) {
    Serial.println(F("[WARN] Sensor อาจแห้งเกินไปหรือขาดการเชื่อมต่อ"));
  }

  return true;
}

// =============================================
// ฟังก์ชัน State Machine (State Management)
// =============================================

void updateSystemState(int moisture) {
  switch (currentState) {
    case SystemState::IDLE:
      // ตรวจสอบว่าต้องเปลี่ยนสถานะหรือไม่
      if (moisture >= MOISTURE_DRY_THRESHOLD) {
        transitionTo(SystemState::WATERING);
      } else if (moisture <= MOISTURE_WET_THRESHOLD) {
        transitionTo(SystemState::VENTILATING);
      }
      // ถ้าความชื้นปกติ ก็อยู่ใน IDLE ต่อ
      break;

    case SystemState::WATERING:
      // ตรวจสอบว่าความชื้นดีขึ้นหรือยัง (ใช้ Hysteresis)
      if (moisture < (MOISTURE_DRY_THRESHOLD - HYSTERESIS)) {
        transitionTo(SystemState::COOLDOWN);
      }
      break;

    case SystemState::VENTILATING:
      // ตรวจสอบว่าความชื้นลดลงหรือยัง (ใช้ Hysteresis)
      if (moisture > (MOISTURE_WET_THRESHOLD + HYSTERESIS)) {
        transitionTo(SystemState::COOLDOWN);
      }
      break;

    case SystemState::COOLDOWN:
      // จะถูกจัดการใน executeState()
      break;
  }
}

void executeState() {
  unsigned long elapsed = getElapsedTime(stateStartTime);

  switch (currentState) {
    case SystemState::IDLE:
      // ไม่ต้องทำอะไร - ระบบพักอยู่
      break;

    case SystemState::WATERING:
      // ตรวจสอบเวลาทำงานของปั๊ม
      if (elapsed >= PUMP_RUN_TIME) {
        Serial.println(F("[PUMP] หยุดปั๊ม (ครบเวลา)"));
        transitionTo(SystemState::COOLDOWN);
      }
      break;

    case SystemState::VENTILATING:
      // ตรวจสอบเวลาทำงานของพัดลม
      if (elapsed >= FAN_RUN_TIME) {
        Serial.println(F("[FAN] หยุดพัดลม (ครบเวลา)"));
        transitionTo(SystemState::COOLDOWN);
      }
      break;

    case SystemState::COOLDOWN:
      // ตรวจสอบว่าพักครบเวลาหรือยัง
      if (elapsed >= COOLDOWN_TIME) {
        Serial.println(F("[COOLDOWN] พักครบเวลา"));
        transitionTo(SystemState::IDLE);
      }
      break;
  }
}

void transitionTo(SystemState newState) {
  if (currentState == newState) {
    return; // ไม่มีการเปลี่ยนแปลง
  }

  // บันทึกสถานะเก่า
  previousState = currentState;

  // หยุดอุปกรณ์เดิมก่อน
  stopAllDevices();

  // แสดงการเปลี่ยนสถานะ
  printStateTransition(currentState, newState);

  // เปลี่ยนสถานะ
  currentState = newState;
  stateStartTime = millis();
  lastStateChangeTime = millis();

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
  }
}

// =============================================
// ฟังก์ชันควบคุมปั๊มน้ำ (Pump Control Functions)
// =============================================

void startPump() {
  Serial.println(F(""));
  Serial.println(F(">>> [PUMP] เปิดปั๊มน้ำ - กำลังรดน้ำ..."));
  digitalWrite(RELAY_PUMP_PIN, RELAY_ON);
}

void stopPump() {
  digitalWrite(RELAY_PUMP_PIN, RELAY_OFF);
}

// =============================================
// ฟังก์ชันควบคุมพัดลม (Fan Control Functions)
// =============================================

void startFan() {
  Serial.println(F(""));
  Serial.println(F(">>> [FAN] เปิดพัดลม - กำลังระบายความชื้น..."));
  digitalWrite(RELAY_FAN_PIN, RELAY_ON);
}

void stopFan() {
  digitalWrite(RELAY_FAN_PIN, RELAY_OFF);
}

// =============================================
// ฟังก์ชันหยุดอุปกรณ์ทั้งหมด (Stop All Devices)
// =============================================

void stopAllDevices() {
  stopPump();
  stopFan();
}

// =============================================
// ฟังก์ชันแสดงผล (Display Functions)
// =============================================

void printSystemStatus(int moisture) {
  Serial.println(F("-------------------------------------"));

  // แสดงค่าความชื้น
  Serial.print(F("Moisture: "));
  Serial.print(moisture);
  Serial.print(F(" | Status: "));
  Serial.println(getMoistureStatus(moisture));

  // แสดงสถานะระบบ
  Serial.print(F("System State: "));
  Serial.print(getStateName(currentState));

  // แสดงเวลาในสถานะปัจจุบัน
  unsigned long elapsed = getElapsedTime(stateStartTime);
  Serial.print(F(" ("));
  Serial.print(elapsed / 1000);
  Serial.println(F("s)"));

  // แสดงสถานะอุปกรณ์
  Serial.print(F("Pump: "));
  Serial.print(currentState == SystemState::WATERING ? F("ON") : F("OFF"));
  Serial.print(F(" | Fan: "));
  Serial.println(currentState == SystemState::VENTILATING ? F("ON") : F("OFF"));

  if (sensorError) {
    Serial.println(F("!!! SENSOR ERROR - Using previous value !!!"));
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
    default:                       return F("UNKNOWN");
  }
}

const __FlashStringHelper* getMoistureStatus(int moisture) {
  if (moisture >= MOISTURE_DRY_THRESHOLD) {
    return F("DRY (ดินแห้ง)");
  } else if (moisture <= MOISTURE_WET_THRESHOLD) {
    return F("TOO WET (ชื้นเกินไป)");
  } else {
    return F("NORMAL (ปกติ)");
  }
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

// =============================================
// ฟังก์ชันสำรองสำหรับ Relay 1 และ 2 (Reserved Functions)
// =============================================

void activateRelay1() {
  digitalWrite(RELAY_1_PIN, RELAY_ON);
  Serial.println(F("[RELAY1] Activated"));
}

void deactivateRelay1() {
  digitalWrite(RELAY_1_PIN, RELAY_OFF);
  Serial.println(F("[RELAY1] Deactivated"));
}

void activateRelay2() {
  digitalWrite(RELAY_2_PIN, RELAY_ON);
  Serial.println(F("[RELAY2] Activated"));
}

void deactivateRelay2() {
  digitalWrite(RELAY_2_PIN, RELAY_OFF);
  Serial.println(F("[RELAY2] Deactivated"));
}
