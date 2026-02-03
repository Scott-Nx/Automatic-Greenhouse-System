/*
 * Sensor.cpp
 * ฟังก์ชันอ่านค่า Sensor ความชื้นในดิน
 *
 * Automatic Greenhouse System v2.2
 */

#include "Sensor.h"
#include "Watchdog.h"
#include "StateMachine.h"

// =============================================
// Private Variables
// =============================================

static uint8_t sensorPin = A0;
static bool sensorErrorFlag = false;
static uint8_t consecutiveErrorCount = 0;

// =============================================
// Sensor Functions Implementation
// =============================================

void sensorInit(uint8_t pin) {
  sensorPin = pin;
  pinMode(sensorPin, INPUT);
  sensorErrorFlag = false;
  consecutiveErrorCount = 0;
  Serial.println(F("[SENSOR] Initialized"));
}

int readSingleSample() {
  return analogRead(sensorPin);
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
    delay(SensorConfig::SAMPLE_DELAY);

    // Reset watchdog ระหว่างอ่านค่า
    if (i % 3 == 0) {
      watchdogReset();
    }
  }

  // ใช้ Median แทน Average เพื่อกรอง spike จาก EMI
  int medianMoisture = getMedianReading(samples, SensorConfig::SAMPLES);

  // ตรวจสอบความถูกต้องของค่า Sensor
  if (!validateSensorReading(medianMoisture)) {
    sensorErrorFlag = true;
    consecutiveErrorCount++;
    return systemData.currentMoisture; // ใช้ค่าเก่าแทน
  }

  sensorErrorFlag = false;
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

int getMoisturePercent(int rawValue) {
  // แปลงค่า Analog (0-1023) เป็นเปอร์เซ็นต์ความชื้น (0-100%)
  // หมายเหตุ: ค่า Analog ต่ำ = ความชื้นสูง (กลับค่า)
  int percent = map(rawValue, SensorConfig::MAX_VALID, SensorConfig::MIN_VALID, 0, 100);

  // จำกัดค่าให้อยู่ในช่วง 0-100
  if (percent < 0) percent = 0;
  if (percent > 100) percent = 100;

  return percent;
}

bool isSensorError() {
  return sensorErrorFlag;
}

void resetSensorError() {
  sensorErrorFlag = false;
}

void incrementConsecutiveErrors() {
  consecutiveErrorCount++;
}

void resetConsecutiveErrors() {
  consecutiveErrorCount = 0;
}

uint8_t getConsecutiveErrors() {
  return consecutiveErrorCount;
}
