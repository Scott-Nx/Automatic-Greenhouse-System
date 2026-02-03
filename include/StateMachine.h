/*
 * StateMachine.h
 * ระบบจัดการสถานะ (State Machine)
 *
 * Automatic Greenhouse System v2.2
 */

#ifndef STATE_MACHINE_H
#define STATE_MACHINE_H

#include <Arduino.h>

// =============================================
// Enum สำหรับสถานะระบบ (System State Enum)
// =============================================

enum class SystemState : uint8_t {
  IDLE,         // โหมดพัก - ความชื้นปกติ
  WATERING,     // กำลังรดน้ำ
  VENTILATING,  // กำลังระบายความชื้น
  COOLDOWN,     // พักหลังทำงาน
  ERROR         // สถานะผิดพลาด (safety mode)
};

// =============================================
// โครงสร้างข้อมูลระบบ (System Data Structure)
// =============================================

struct SystemData {
  SystemState currentState;
  SystemState previousState;

  unsigned long lastReadTime;
  unsigned long stateStartTime;
  unsigned long lastStateChangeTime;
  unsigned long lastLcdUpdateTime;

  int currentMoisture;
  int previousMoisture;

  bool sensorError;
  uint8_t consecutiveErrors;
  uint8_t wdtResetCount;

  // Constructor with default values
  SystemData()
    : currentState(SystemState::IDLE)
    , previousState(SystemState::IDLE)
    , lastReadTime(0)
    , stateStartTime(0)
    , lastStateChangeTime(0)
    , lastLcdUpdateTime(0)
    , currentMoisture(512)
    , previousMoisture(512)
    , sensorError(false)
    , consecutiveErrors(0)
    , wdtResetCount(0)
  {}

  // Reset to initial state
  void reset() {
    currentState = SystemState::IDLE;
    previousState = SystemState::IDLE;
    lastReadTime = 0;
    stateStartTime = millis();
    lastStateChangeTime = millis();
    lastLcdUpdateTime = 0;
    currentMoisture = 512;
    previousMoisture = 512;
    sensorError = false;
    consecutiveErrors = 0;
  }
};

// =============================================
// Global System Data (extern declaration)
// =============================================

extern SystemData systemData;

// =============================================
// Function Declarations
// =============================================

/**
 * @brief อัพเดทสถานะระบบตามค่าความชื้น
 * @param moisture ค่าความชื้นจาก sensor
 */
void updateSystemState(int moisture);

/**
 * @brief ดำเนินการตามสถานะปัจจุบัน
 */
void executeState();

/**
 * @brief เปลี่ยนไปยังสถานะใหม่
 * @param newState สถานะใหม่ที่ต้องการ
 */
void transitionTo(SystemState newState);

/**
 * @brief จัดการเมื่อเกิดข้อผิดพลาด
 */
void handleError();

/**
 * @brief รับชื่อสถานะสำหรับแสดงผล
 * @param state สถานะที่ต้องการชื่อ
 * @return ชื่อสถานะเป็น Flash String
 */
const __FlashStringHelper* getStateName(SystemState state);

#endif // STATE_MACHINE_H
