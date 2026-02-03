/*
 * Sensor.h
 * ฟังก์ชันอ่านค่า Sensor ความชื้นในดิน
 *
 * Automatic Greenhouse System v2.2
 */

#ifndef SENSOR_H
#define SENSOR_H

#include <Arduino.h>
#include "Config.h"

// =============================================
// Sensor Functions
// =============================================

/**
 * @brief เริ่มต้น Sensor
 * @param pin ขา Analog ที่ต่อกับ Sensor
 */
void sensorInit(uint8_t pin);

/**
 * @brief อ่านค่าความชื้นจาก Sensor
 * @return ค่าความชื้น (0-1023) หรือค่าเก่าถ้าอ่านไม่สำเร็จ
 */
int readSoilMoisture();

/**
 * @brief อ่านค่าจาก Sensor หนึ่งครั้ง
 * @return ค่า Analog (0-1023)
 */
int readSingleSample();

/**
 * @brief หาค่า Median จาก Array
 * @param samples Array ของค่าที่อ่านได้
 * @param count จำนวนค่าใน Array
 * @return ค่า Median
 */
int getMedianReading(int* samples, int count);

/**
 * @brief ตรวจสอบความถูกต้องของค่าที่อ่านได้
 * @param reading ค่าที่อ่านได้
 * @return true ถ้าค่าถูกต้อง, false ถ้าไม่ถูกต้อง
 */
bool validateSensorReading(int reading);

/**
 * @brief ตรวจสอบว่าค่าเปลี่ยนแปลงมากผิดปกติหรือไม่ (EMI spike)
 * @param newReading ค่าใหม่
 * @param oldReading ค่าเก่า
 * @return true ถ้าเป็น spike, false ถ้าปกติ
 */
bool checkEmiSpike(int newReading, int oldReading);

/**
 * @brief แปลงค่า Analog เป็นเปอร์เซ็นต์ความชื้น
 * @param rawValue ค่า Analog (0-1023)
 * @return เปอร์เซ็นต์ความชื้น (0-100%)
 */
int getMoisturePercent(int rawValue);

/**
 * @brief ตรวจสอบว่า Sensor มีข้อผิดพลาดหรือไม่
 * @return true ถ้ามีข้อผิดพลาด
 */
bool isSensorError();

/**
 * @brief รีเซ็ตสถานะข้อผิดพลาดของ Sensor
 */
void resetSensorError();

/**
 * @brief เพิ่มจำนวน Consecutive Error
 */
void incrementConsecutiveErrors();

/**
 * @brief รีเซ็ตจำนวน Consecutive Error
 */
void resetConsecutiveErrors();

/**
 * @brief ดึงจำนวน Consecutive Error
 * @return จำนวน Consecutive Error
 */
uint8_t getConsecutiveErrors();

#endif // SENSOR_H
