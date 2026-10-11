#include "offset.h"
#include "communication.h"
#include "kinematics.h"
#include "robot_state.h"
#include "servo_movement.h"

// Target X Y Z = current TCP position + offsets from the command.
static bool offset_target(const char* line, float* xyz, float& speed, float& accel) {
  float delta[MAX_JOINTS];
  if (!parse_motion(line, delta, speed, accel) || !require_kinematics()) return false;
  for (uint8_t a = 0; a < 3; a++) xyz[a] = robot.axis[a] + delta[a];
  return true;
}

void cmd_offset_j(const char* line) {
  float xyz[3], speed, accel;
  if (offset_target(line, xyz, speed, accel)) {
    float target[MAX_JOINTS];
    memcpy(target, robot.jointDeg, sizeof(target));
    if (kin_inverse(xyz, target)) motion_move_joints(target, speed, accel);
    else comm_send(F("ERROR: target position is out of reach - move refused"));
  }
  send_position();
}

void cmd_offset_l(const char* line) {
  float xyz[3], speed, accel;
  if (offset_target(line, xyz, speed, accel)) motion_move_linear(xyz, speed, accel);
  send_position();
}
