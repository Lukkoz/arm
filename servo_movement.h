#pragma once
#include <Arduino.h>

// Servo movement section: the motion engine and the joint / absolute move commands.

// Joint interpolated move with a trapezoidal speed profile. Every joint arrives at the same time,
// the slowest joint sets the duration. Returns false if refused or stopped.
bool motion_move_joints(const float* targetDeg, float speedPct, float accelPct);

// Straight line move of the TCP to target X Y Z (3-axis kinematics).
bool motion_move_linear(const float* targetXyz, float speedPct, float accelPct);

// Parses "<cmd> A..B..C..<speed><accel>": numJoints values, then optional speed and acceleration in %.
bool parse_motion(const char* line, float* values, float& speedPct, float& accelPct);

bool require_kinematics();   // reports an error when Cartesian moves are not available
void send_position();        // "POS: X .. Y .. Z .. J1 .. J2 .. J3 .."

void cmd_move_joint(const char* line);  // MoveJoint: absolute joint angles
void cmd_jog_joint(const char* line);   // jogJ: relative joint angles
void cmd_move_j(const char* line);      // MoveJ: absolute X Y Z, joint interpolated
void cmd_move_l(const char* line);      // MoveL: absolute X Y Z, straight line

// Wrist roll servo, outside the kinematics: 0 = gripper straight, limits WRIST_ROLL_MIN/MAX_DEG.
bool motion_move_roll(float targetDeg, float speedPct, float accelPct);
void cmd_wrist_roll(const char* line);  // WristRoll A<deg> [B<speed %>] [C<accel %>]

// Wrist pitch (extra joint servo): gripper angle to the horizontal, 0 = level, + = up.
// Part of the kinematics: MoveJ / MoveL keep the gripper at this angle.
bool motion_move_pitch(float targetDeg, float speedPct, float accelPct);
void cmd_wrist_pitch(const char* line); // WristPitch A<deg> [B<speed %>] [C<accel %>]
