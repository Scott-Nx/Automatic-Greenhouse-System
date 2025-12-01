/*
 * ระบบโรงเรือนอัตโนมัติ (Automatic Greenhouse System)
 * สำหรับบอร์ด Arduino Uno
 *
 * ระบบควบคุมความชื้นในดินอัตโนมัติ:
 * - เมื่อความชื้นต่ำ: เปิดปั๊มน้ำเพื่อรดน้ำ
 * - เมื่อความชื้นสูง: เปิดพัดลมเพื่อดูดความชื้นออก
 */

// =============================================
// การกำหนดขา (Pin Definitions)
// =============================================

// Sensor วัดความชื้นในดิน
#include <Arduino.h>
#define SOIL_MOISTURE_PIN A0  // ขา Analog สำหรับอ่านค่าความชื้น

// Relay 4-Channel (Active-Low: LOW = เปิด, HIGH = ปิด)
#define RELAY_PUMP_PIN    2   // IN1 - ควบคุมปั๊มน้ำ
#define RELAY_FAN_PIN     3   // IN2 - ควบคุมพัดลม (รอต่อใช้งาน)
#define RELAY_3_PIN       4   // IN3 - รอต่อใช้งาน
#define RELAY_4_PIN       5   // IN4 - รอต่อใช้งาน

// =============================================
// ค่าคงที่สำหรับการตั้งค่า (Configuration Constants)
// =============================================

// ค่าความชื้น (0-1023 จาก Analog Read)
// หมายเหตุ: ค่าต่ำ = ความชื้นสูง, ค่าสูง = ความชื้นต่ำ (สำหรับ Sensor ส่วนใหญ่)
#define MOISTURE_DRY_THRESHOLD    700   // ค่าขีดจำกัดดินแห้ง (ต้องรดน้ำ)
#define MOISTURE_WET_THRESHOLD    300   // ค่าขีดจำกัดดินชื้นเกินไป (ต้องเปิดพัดลม)

// ค่า Hysteresis เพื่อป้องกันการสั่นสะเทือนของระบบ
#define HYSTERESIS                50

// ระยะเวลาในการทำงาน (มิลลิวินาที)
#define PUMP_RUN_TIME             5000  // เวลาเปิดปั๊มน้ำ 5 วินาที
#define FAN_RUN_TIME              10000 // เวลาเปิดพัดลม 10 วินาที
#define READ_INTERVAL             2000  // อ่านค่า Sensor ทุก 2 วินาที

// สถานะ Relay (Active-Low)
#define RELAY_ON                  LOW
#define RELAY_OFF                 HIGH

// =============================================
// ตัวแปรสถานะ (State Variables)
// =============================================
unsigned long lastReadTime = 0;
unsigned long pumpStartTime = 0;
unsigned long fanStartTime = 0;

bool isPumpRunning = false;
bool isFanRunning = false;

int currentMoisture = 0;

// =============================================
// ฟังก์ชันหลัก (Main Functions)
// =============================================

void setup() {
  // เริ่มต้น Serial Monitor สำหรับ Debug
  Serial.begin(9600);
  Serial.println(F("====================================="));
  Serial.println(F("ระบบโรงเรือนอัตโนมัติ เริ่มทำงาน"));
  Serial.println(F("Automatic Greenhouse System Started"));
  Serial.println(F("====================================="));

  // ตั้งค่าขา Relay เป็น Output
  pinMode(RELAY_PUMP_PIN, OUTPUT);
  pinMode(RELAY_FAN_PIN, OUTPUT);
  pinMode(RELAY_3_PIN, OUTPUT);
  pinMode(RELAY_4_PIN, OUTPUT);

  // ปิด Relay ทั้งหมดตอนเริ่มต้น (Active-Low: HIGH = ปิด)
  digitalWrite(RELAY_PUMP_PIN, RELAY_OFF);
  digitalWrite(RELAY_FAN_PIN, RELAY_OFF);
  digitalWrite(RELAY_3_PIN, RELAY_OFF);
  digitalWrite(RELAY_4_PIN, RELAY_OFF);

  // ตั้งค่าขา Sensor เป็น Input
  pinMode(SOIL_MOISTURE_PIN, INPUT);

  Serial.println(F("เริ่มต้นระบบสำเร็จ!"));
  Serial.println();
}

void loop() {
  unsigned long currentTime = millis();

  // อ่านค่า Sensor ตามช่วงเวลาที่กำหนด
  if (currentTime - lastReadTime >= READ_INTERVAL) {
    lastReadTime = currentTime;

    // อ่านค่าความชื้นจาก Sensor
    currentMoisture = readSoilMoisture();

    // แสดงค่าบน Serial Monitor
    printMoistureStatus(currentMoisture);

    // ตรวจสอบและควบคุมระบบ
    controlSystem(currentMoisture);
  }

  // ตรวจสอบการหยุดทำงานของปั๊มน้ำ
  if (isPumpRunning && (currentTime - pumpStartTime >= PUMP_RUN_TIME)) {
    stopPump();
  }

  // ตรวจสอบการหยุดทำงานของพัดลม
  if (isFanRunning && (currentTime - fanStartTime >= FAN_RUN_TIME)) {
    stopFan();
  }
}

// =============================================
// ฟังก์ชันอ่านค่า Sensor (Sensor Reading Functions)
// =============================================

int readSoilMoisture() {
  // อ่านค่าหลายครั้งแล้วหาค่าเฉลี่ย เพื่อลด Noise
  int sum = 0;
  const int samples = 10;

  for (int i = 0; i < samples; i++) {
    sum += analogRead(SOIL_MOISTURE_PIN);
    delay(10);
  }

  return sum / samples;
}

// =============================================
// ฟังก์ชันควบคุมระบบ (Control Functions)
// =============================================

void controlSystem(int moisture) {
  // กรณีดินแห้ง (ค่าสูง) - เปิดปั๊มน้ำ
  if (moisture >= MOISTURE_DRY_THRESHOLD && !isPumpRunning && !isFanRunning) {
    startPump();
  }

  // กรณีดินชื้นเกินไป (ค่าต่ำ) - เปิดพัดลม
  if (moisture <= MOISTURE_WET_THRESHOLD && !isFanRunning && !isPumpRunning) {
    startFan();
  }
}

// =============================================
// ฟังก์ชันควบคุมปั๊มน้ำ (Pump Control Functions)
// =============================================

void startPump() {
  Serial.println(F(">>> เปิดปั๊มน้ำ - กำลังรดน้ำ..."));
  digitalWrite(RELAY_PUMP_PIN, RELAY_ON);
  isPumpRunning = true;
  pumpStartTime = millis();
}

void stopPump() {
  Serial.println(F(">>> ปิดปั๊มน้ำ"));
  digitalWrite(RELAY_PUMP_PIN, RELAY_OFF);
  isPumpRunning = false;
}

// =============================================
// ฟังก์ชันควบคุมพัดลม (Fan Control Functions)
// =============================================

void startFan() {
  Serial.println(F(">>> เปิดพัดลม - กำลังดูดความชื้น..."));
  digitalWrite(RELAY_FAN_PIN, RELAY_ON);
  isFanRunning = true;
  fanStartTime = millis();
}

void stopFan() {
  Serial.println(F(">>> ปิดพัดลม"));
  digitalWrite(RELAY_FAN_PIN, RELAY_OFF);
  isFanRunning = false;
}

// =============================================
// ฟังก์ชันแสดงผล (Display Functions)
// =============================================

void printMoistureStatus(int moisture) {
  Serial.print(F("ค่าความชื้น (Moisture): "));
  Serial.print(moisture);
  Serial.print(F(" | สถานะ: "));

  if (moisture >= MOISTURE_DRY_THRESHOLD) {
    Serial.println(F("ดินแห้ง (DRY)"));
  } else if (moisture <= MOISTURE_WET_THRESHOLD) {
    Serial.println(F("ดินชื้นมาก (TOO WET)"));
  } else {
    Serial.println(F("ปกติ (NORMAL)"));
  }

  // แสดงสถานะอุปกรณ์
  Serial.print(F("  ปั๊มน้ำ: "));
  Serial.print(isPumpRunning ? F("ทำงาน") : F("ปิด"));
  Serial.print(F(" | พัดลม: "));
  Serial.println(isFanRunning ? F("ทำงาน") : F("ปิด"));
  Serial.println();
}

// =============================================
// ฟังก์ชันสำรองสำหรับ Relay 3 และ 4 (Reserved Functions)
// =============================================

void activateRelay3() {
  digitalWrite(RELAY_3_PIN, RELAY_ON);
}

void deactivateRelay3() {
  digitalWrite(RELAY_3_PIN, RELAY_OFF);
}

void activateRelay4() {
  digitalWrite(RELAY_4_PIN, RELAY_ON);
}

void deactivateRelay4() {
  digitalWrite(RELAY_4_PIN, RELAY_OFF);
}
