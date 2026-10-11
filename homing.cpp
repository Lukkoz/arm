#include "homing.h"
#include "communication.h"
#include "robot_state.h"
#include "servo_movement.h"
#include "servo_output.h"

static bool home_joint(uint8_t joint) {
  float target[MAX_JOINTS];
  memcpy(target, robot.jointDeg, sizeof(target));
  target[joint] = robot.joint[joint].homeDeg;
  return motion_move_joints(target, HOMING_SPEED_PCT, HOMING_ACCEL_PCT);
}

void cmd_home(const char*) {
  if (!servo_output_present()) servo_output_begin();   // retry, e.g. driver powered late

  bool ok = true;
  bool done[MAX_JOINTS] = {false};

  // joints with a home order 1..n, in that order
  for (uint8_t order = 1; ok && order <= robot.numJoints; order++) {
    for (uint8_t j = 0; ok && j < robot.numJoints; j++) {
      if (!done[j] && robot.joint[j].homeOrder == order) {
        ok = home_joint(j);
        done[j] = true;
      }
    }
  }
  // joints without a valid order last
  for (uint8_t j = 0; ok && j < robot.numJoints; j++) {
    if (!done[j]) ok = home_joint(j);
  }
  if (ok) ok = motion_move_roll(0.0f, HOMING_SPEED_PCT, HOMING_ACCEL_PCT);   // gripper straight
  if (ok && robot.extraJoint)                                                // gripper level
    ok = motion_move_pitch(0.0f, HOMING_SPEED_PCT, HOMING_ACCEL_PCT);

  robot.homed = ok;
  if (ok) comm_send(F("home"));
  send_position();
}
