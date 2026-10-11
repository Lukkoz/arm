#include "robot_setup.h"
#include "command_parser.h"
#include "communication.h"
#include "kinematics.h"
#include "robot_state.h"

#define MAX_VALUES (MAX_DH_ROWS * 4)

// Parses the values and checks that exactly `expected` were sent.
static bool read_values(const char* line, float* v, uint8_t expected) {
  uint8_t got = parse_values(line, v, MAX_VALUES);
  if (got == expected) return true;
  char msg[80];
  const char* end = strchr(line, ' ');
  int nameLength = end ? int(end - line) : int(strlen(line));
  snprintf_P(msg, sizeof(msg), PSTR("ERROR: %.*s expects %u values, got %u - ignored"),
             nameLength, line, expected, got);
  comm_send(msg);
  return false;
}

static void set_number_of_joints(const char* line) {
  float v[MAX_VALUES];
  if (!read_values(line, v, 1)) return;
  int n = int(v[0]);
  if (n < 1 || n > MAX_JOINTS) {
    comm_send(F("ERROR: number of joints must be 1-6"));
    return;
  }
  if (n != robot.numJoints) {          // a different robot: drop the old wrist setting
    robot.numJoints = n;
    robot.extraJoint = false;
  }
  kin_update_position();
  comm_send(F("Number of joints is set"));
}

static void set_extra_joint(const char* line) {
  float v[MAX_VALUES];
  if (!read_values(line, v, 1)) return;
  robot.extraJoint = v[0] != 0 && robot.numJoints < MAX_JOINTS;
  kin_update_position();
  comm_send(F("Extra joint is set"));
}

static void set_motor_type(const char* line) {
  float v[MAX_VALUES];
  if (!read_values(line, v, robot.numJoints)) return;
  for (uint8_t i = 0; i < robot.numJoints; i++) {
    if (v[i] == 0) {
      comm_send(F("WARNING: stepper motors are not supported, all joints are driven as servos"));
      return;
    }
  }
  comm_send(F("Motor types are set"));
}

static void set_servo_pin(const char* line) {
  float v[MAX_VALUES];
  if (!read_values(line, v, robot.numJoints)) return;
  for (uint8_t i = 0; i < robot.numJoints; i++) {
    if (v[i] < 0 || v[i] > 15) {
      comm_send(F("ERROR: servo pins are PCA9685 channels 0-15 - ignored"));
      return;
    }
  }
  for (uint8_t i = 0; i < robot.numJoints; i++) robot.joint[i].channel = uint8_t(v[i]);
  comm_send(F("Servo channels are set"));
}

static void set_servo_pulse(const char* line) {
  float v[MAX_VALUES];
  if (!read_values(line, v, robot.numJoints * 3)) return;
  for (uint8_t i = 0; i < robot.numJoints; i++) {
    float hi = v[i * 3], lo = v[i * 3 + 1], range = v[i * 3 + 2];
    if (range <= 0 || lo < 0 || hi <= 0) {
      comm_send(F("ERROR: invalid servo pulse settings - ignored"));
      return;
    }
  }
  for (uint8_t i = 0; i < robot.numJoints; i++) {
    robot.joint[i].pulseMaxUs = uint16_t(v[i * 3]);
    robot.joint[i].pulseMinUs = uint16_t(v[i * 3 + 1]);
    robot.joint[i].rangeDeg = v[i * 3 + 2];
  }
  comm_send(F("Servo pulse settings are set"));
}

static void set_servo_position(const char* line) {
  float v[MAX_VALUES];
  if (!read_values(line, v, robot.numJoints)) return;
  for (uint8_t i = 0; i < robot.numJoints; i++) robot.joint[i].zeroDeg = v[i];
  comm_send(F("Servo zero positions are set"));
}

static void set_dir_joints(const char* line) {
  float v[MAX_VALUES];
  if (!read_values(line, v, robot.numJoints)) return;
  for (uint8_t i = 0; i < robot.numJoints; i++) robot.joint[i].dir = v[i] != 0 ? -1 : 1;
  comm_send(F("Joint directions are set"));
}

static void set_max_pos(const char* line) {
  float v[MAX_VALUES];
  if (!read_values(line, v, robot.numJoints * 2)) return;
  for (uint8_t i = 0; i < robot.numJoints; i++) {
    float a = v[i * 2], b = v[i * 2 + 1];          // Studio order: min, max
    robot.joint[i].minDeg = min(a, b);
    robot.joint[i].maxDeg = max(a, b);
  }
  comm_send(F("Joint limits are set"));
}

static void set_speed(const char* line) {
  float v[MAX_VALUES];
  if (!read_values(line, v, robot.numJoints * 2)) return;
  for (uint8_t i = 0; i < robot.numJoints; i++) {
    if (v[i * 2] > 0) robot.joint[i].speedDegS = v[i * 2];
    if (v[i * 2 + 1] > 0) robot.joint[i].accelDegS2 = v[i * 2 + 1];
  }
  comm_send(F("Speed settings are set"));
}

static void set_home_settings(const char* line) {
  float v[MAX_VALUES];
  if (!read_values(line, v, robot.numJoints * 2)) return;
  for (uint8_t i = 0; i < robot.numJoints; i++) {
    robot.joint[i].homeOrder = uint8_t(v[i * 2]);
    robot.joint[i].homeDeg = v[i * 2 + 1];
  }
  comm_send(F("Homing settings are set"));
}

static void set_dh_par(const char* line) {
  float v[MAX_VALUES];
  uint8_t got = parse_values(line, v, MAX_VALUES);
  uint8_t rows = got / 4;
  if (got % 4 != 0 || rows < robot.numJoints || rows > robot.numJoints + 1) {
    comm_send(F("ERROR: Set_dh_par needs 4 values per joint (+4 for the extra joint) - ignored"));
    return;
  }
  for (uint8_t r = 0; r < rows; r++)
    for (uint8_t c = 0; c < 4; c++) robot.dh[r][c] = v[r * 4 + c];
  robot.extraJoint = rows > robot.numJoints;
  kin_update_position();
  comm_send(F("DH parameters are set"));
}

static void set_tool_frame(const char* line) {
  float v[MAX_VALUES];
  if (!read_values(line, v, 6)) return;
  memcpy(robot.toolFrame, v, sizeof(robot.toolFrame));
  kin_update_position();
  comm_send(F("Tool frame is set"));
}

static void set_tools(const char* line) {
  // A channel, B type, C servo min, D servo max
  float v[MAX_VALUES];
  if (parse_values(line, v, 4) < 4) {
    comm_send(F("ERROR: Set_tools needs channel, type, min, max - ignored"));
    return;
  }
  if (v[0] >= 0 && v[0] <= 15) robot.toolChannel = uint8_t(v[0]);
  if (v[3] > v[2]) {
    robot.toolMinDeg = v[2];
    robot.toolMaxDeg = v[3];
  }
  comm_send(F("Tool settings are set"));
}

bool setup_command(const char* line) {
  if (cmd_is(line, PSTR("Set_number_of_joints")))   set_number_of_joints(line);
  else if (cmd_is(line, PSTR("Set_extra_joint")))   set_extra_joint(line);
  else if (cmd_is(line, PSTR("Set_motor_type")))    set_motor_type(line);
  else if (cmd_is(line, PSTR("Set_servo_pin")))     set_servo_pin(line);
  else if (cmd_is(line, PSTR("Set_servo_pulse")))   set_servo_pulse(line);
  else if (cmd_is(line, PSTR("Set_servo_position"))) set_servo_position(line);
  else if (cmd_is(line, PSTR("Set_dir_joints")))    set_dir_joints(line);
  else if (cmd_is(line, PSTR("Set_max_pos")))       set_max_pos(line);
  else if (cmd_is(line, PSTR("Set_speed")))         set_speed(line);
  else if (cmd_is(line, PSTR("Set_home_settings"))) set_home_settings(line);
  else if (cmd_is(line, PSTR("Set_dh_par")))        set_dh_par(line);
  else if (cmd_is(line, PSTR("Set_tool_frame")))    set_tool_frame(line);
  else if (cmd_is(line, PSTR("Set_tools")))         set_tools(line);
  else if (cmd_is(line, PSTR("Set_io_pin")))
    comm_send(F("IO pins need no setup, IO_digitalWrite configures the pin"));
  else if (cmd_is(line, PSTR("Set_motor_pin")) || cmd_is(line, PSTR("Set_ena_pin")) ||
           cmd_is(line, PSTR("Set_switch_pin")) || cmd_is(line, PSTR("Set_lim_pos")) ||
           cmd_is(line, PSTR("Set_step_deg")))
    comm_send(F("Stepper setting ignored (servo firmware)"));
  else
    return false;
  return true;
}
