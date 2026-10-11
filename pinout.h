#pragma once
#include <Arduino.h>
#include "config.h"

// =====================================================================
//  Pinout and hardware calibration
// =====================================================================

// ---- I2C bus to the PCA9685 servo driver ---------------------------------
#if defined(CONFIG_IDF_TARGET_ESP32S3)
  #define I2C_SDA_PIN SDA          // the board's I2C pins: XIAO ESP32S3 D4 (GPIO5) / D5 (GPIO6),
  #define I2C_SCL_PIN SCL          // generic ESP32-S3 GPIO8 / GPIO9; any free GPIO can be used
#elif defined(CONTROLLER_ESP32)
  #define I2C_SDA_PIN 21           // ESP32 default I2C pins; any free GPIO can be used
  #define I2C_SCL_PIN 22
#else
  // AVR boards use the fixed hardware TWI pins (Uno/Nano: A4/A5, Mega: 20/21)
  #define I2C_SDA_PIN SDA
  #define I2C_SCL_PIN SCL
#endif
#define I2C_CLOCK_HZ        100000UL    // standard mode, as the Adafruit library; breakout pull-ups are weak (10k)

// ---- PCA9685 ----------------------------------------------------------------
#define PCA9685_I2C_ADDRESS 0x7F        // the arm's driver: address pins A0-A5 all high (default boards: 0x40)
#define PCA9685_OSC_HZ      25000000UL  // trim if the measured servo frame is off (boards vary 25-27 MHz)
#define PCA9685_OE_PIN      -1          // output enable pin (active low), -1 = not connected
#define SERVO_FREQUENCY_HZ  50

// ---- Servo channels on the PCA9685 (0-15) ---------------------------------------
// Joints J1..J6. The MiKoBots Studio "servo pin" settings override these.
static const uint8_t JOINT_SERVO_CHANNEL[6] = {0, 1, 2, 3, 4, 5};
#define TOOL_SERVO_CHANNEL  4       // gripper

// ---- Wrist of the ARM -----------------------------------------------------------------
#define WRIST_PITCH_SERVO_CHANNEL 5 // extra joint: gripper up/down, J4 = -(J2 + J3) + pitch (WristPitch)
#define WRIST_PITCH_MIN_DEG -90.0f  // angle of the gripper to the horizontal, 0 = level, + = up
#define WRIST_PITCH_MAX_DEG  90.0f
#define WRIST_ROLL_SERVO_CHANNEL  3 // turns the gripper about its own axis (WristRoll)
#define WRIST_ROLL_MIN_DEG  -90.0f  // 0 = servo at SERVO_ZERO_DEG, gripper straight
#define WRIST_ROLL_MAX_DEG   90.0f

// ---- Default servo calibration (overridden by the Studio servo settings) ------------
#define SERVO_PULSE_MIN_US  500     // pulse at servo angle 0
#define SERVO_PULSE_MAX_US  2500    // pulse at servo angle SERVO_RANGE_DEG
#define SERVO_RANGE_DEG     180
#define SERVO_ZERO_DEG      90      // servo angle at joint angle 0

// ---- Tool ----------------------------------------------------------------------------
#define TOOL_SERVO_MIN_DEG  40      // gripper servo angle at Tool_move_to pos(0)
#define TOOL_SERVO_MAX_DEG  120     // gripper servo angle at Tool_move_to pos(100)
#if defined(CONFIG_IDF_TARGET_ESP32S3)
  #define TOOL_RELAY_PIN    4       // relay / vacuum tool output (Tool_state), XIAO D3; GPIO 26-32 are flash pins on the S3
#elif defined(CONTROLLER_ESP32)
  #define TOOL_RELAY_PIN    26      // relay / vacuum tool output (Tool_state)
#else
  #define TOOL_RELAY_PIN    7
#endif
#define TOOL_RELAY_ACTIVE_LOW 1     // relay modules usually switch on with a LOW input
