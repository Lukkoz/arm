// =====================================================================
//  ARM robot controller - firmware for MiKoBots Studio
//
//  Servos are driven through a PCA9685 I2C servo driver.
//
//  config.h          target controller (ESP32 / Arduino), features, defaults
//  pinout.h          I2C pins, PCA9685 address, servo channels, calibration
//  communication     serial / BLE link and command dispatch
//  robot_setup       setup section: Set_... settings from the Studio
//  servo_movement    servo movement section: motion engine, MoveJoint, jogJ, MoveJ, MoveL
//  offset            offset section: OffsetJ, OffsetL, jogL
//  homing            homing section: Home
//  tool_io           gripper, relay tool and digital outputs
//  kinematics        3-axis forward / inverse kinematics
//  servo_output      joint angles -> servo pulses, pca9685: I2C driver
// =====================================================================
#include "config.h"
#include "communication.h"
#include "kinematics.h"
#include "robot_state.h"
#include "servo_output.h"
#include "tool_io.h"

void setup() {
  robot_state_init();
  comm_begin();
  tool_io_begin();
  servo_output_begin();
  kin_update_position();

  delay(100);
  comm_send(F("ROBOT_CONNECTED"));
  servo_output_report();
}

void loop() {
  comm_poll();
}
