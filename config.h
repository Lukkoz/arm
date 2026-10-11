#pragma once
// =====================================================================
//  Build configuration
// =====================================================================

// ---- Target controller: enable exactly ONE ----------------------------
#define CONTROLLER_ESP32
// #define CONTROLLER_ARDUINO      // AVR boards: Uno, Nano, Mega

// ---- Communication -------------------------------------------------------
#define SERIAL_BAUD        115200   // must match MiKoBots Studio
#define USE_BLE            1       // ESP32 only: Bluetooth LE link to MiKoBots Studio
#define BLE_DEVICE_NAME    "ARM_TEST"

// ---- Robot defaults ---------------------------------------------------------
// Used until MiKoBots Studio sends its settings on connect.
#define DEFAULT_NUMBER_OF_JOINTS 3
#define DEFAULT_EXTRA_JOINT      1
// 0: the elbow servo sits in the elbow and turns with the upper arm (ARM)
// 1: the elbow servo angle is measured against the base (parallelogram drive, MiKo mini)
#define ELBOW_COUPLED            0

// ---- Motion -------------------------------------------------------------------
#define SERVO_UPDATE_MS           20      // interpolation step = one servo frame at 50 Hz
#define DEFAULT_JOINT_SPEED_DEG_S   90.0f   // joint speed at 100 % until Set_speed arrives
#define DEFAULT_JOINT_ACCEL_DEG_S2  360.0f  // joint acceleration at 100 % until Set_speed arrives
#define LINEAR_SPEED_MM_S           150.0f  // TCP speed of linear moves at 100 %
#define LINEAR_ACCEL_MM_S2          600.0f  // TCP acceleration of linear moves at 100 %
#define HOMING_SPEED_PCT            30.0f
#define HOMING_ACCEL_PCT            30.0f
#define DEFAULT_SPEED_PCT           50.0f   // used when a command has no speed / acceleration

// =====================================================================
//  Derived settings - normally nothing to edit below this line
// =====================================================================
#if defined(CONTROLLER_ESP32) && defined(CONTROLLER_ARDUINO)
  #error "config.h: select only one of CONTROLLER_ESP32 / CONTROLLER_ARDUINO"
#elif !defined(CONTROLLER_ESP32) && !defined(CONTROLLER_ARDUINO)
  #error "config.h: select CONTROLLER_ESP32 or CONTROLLER_ARDUINO"
#endif

#if defined(CONTROLLER_ESP32)
  #if !defined(ARDUINO_ARCH_ESP32)
    #error "config.h selects CONTROLLER_ESP32, but the board selected for the build is not an ESP32"
  #endif
  #define LINE_BUFFER_SIZE 256
  #define BLE_ENABLED      USE_BLE
#else
  #if !defined(ARDUINO_ARCH_AVR)
    #error "config.h selects CONTROLLER_ARDUINO, but the board selected for the build is not an AVR Arduino"
  #endif
  #define LINE_BUFFER_SIZE 160
  #define BLE_ENABLED      0
#endif
