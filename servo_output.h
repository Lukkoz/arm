#pragma once
#include <Arduino.h>

// Servo hardware layer: joint angles -> servo angles -> PCA9685 pulses.

bool servo_output_begin();      // starts I2C and the PCA9685, servos stay limp
bool servo_output_present();    // PCA9685 found on the I2C bus
// When the PCA9685 was not at its configured address: sends the I2C scan result and
// which address is used (the only device on the bus), or why none is.
void servo_output_report();

// Writes all joints (and the extra wrist joint) for the given joint angles.
// The first write switches the servo outputs on.
void servo_output_write_joints(const float* jointDeg);
void servo_output_write_tool(float servoDeg);
void servo_output_write_roll(float rollDeg);   // wrist roll, 0 = gripper straight
