#pragma once
#include <Arduino.h>
#include "config.h"

#define MAX_JOINTS   6
#define MAX_DH_ROWS  (MAX_JOINTS + 1)
#define NUM_AXES     6

// Per joint servo settings, all set by MiKoBots Studio on connect.
struct JointConfig {
  float    minDeg, maxDeg;        // Set_max_pos
  float    speedDegS;             // Set_speed, joint speed at 100 %
  float    accelDegS2;            // Set_speed, joint acceleration at 100 %
  float    zeroDeg;               // Set_servo_position, servo angle at joint angle 0
  int8_t   dir;                   // Set_dir_joints, +1 or -1
  uint16_t pulseMinUs, pulseMaxUs;
  float    rangeDeg;              // Set_servo_pulse
  uint8_t  channel;               // Set_servo_pin, PCA9685 channel
  uint8_t  homeOrder;             // Set_home_settings
  float    homeDeg;
};

struct RobotState {
  uint8_t numJoints;
  bool    extraJoint;             // wrist pitch joint: J(n+1) = -(J2 + J3) + pitchDeg
  JointConfig joint[MAX_JOINTS];  // joint[numJoints] holds the extra joint servo
  float   dh[MAX_DH_ROWS][4];     // theta, alpha, d, a
  float   toolFrame[6];           // x, y, z, rz, ry, rx
  uint8_t toolChannel;
  float   toolMinDeg, toolMaxDeg;
  JointConfig roll;               // wrist roll servo (WristRoll), not part of the kinematics
  JointConfig pitch;              // limits and speed of WristPitch; the servo is joint[numJoints]

  float   jointDeg[MAX_JOINTS];   // commanded joint angles
  float   rollDeg;                // commanded wrist roll angle
  float   pitchDeg;               // commanded gripper angle to the horizontal, 0 = level
  float   axis[NUM_AXES];         // TCP pose from forward kinematics
  bool    homed;
  bool    stopRequested;
  bool    paused;
};

extern RobotState robot;

void robot_state_init();
