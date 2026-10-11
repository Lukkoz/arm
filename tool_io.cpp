#include "tool_io.h"
#include "command_parser.h"
#include "communication.h"
#include "pinout.h"
#include "robot_state.h"
#include "servo_output.h"

static void write_relay(bool on) {
  digitalWrite(TOOL_RELAY_PIN, (on != (TOOL_RELAY_ACTIVE_LOW != 0)) ? HIGH : LOW);
}

void tool_io_begin() {
  pinMode(TOOL_RELAY_PIN, OUTPUT);
  write_relay(false);
}

void cmd_tool_move_to(const char* line) {
  const char* p = strchr(line, '(');
  if (!p) {
    comm_send(F("ERROR: expected Tool_move_to pos(0-100)"));
    return;
  }
  float pos = constrain(atof(p + 1), 0.0f, 100.0f);
  if (!servo_output_present())
    comm_send(F("WARNING: PCA9685 not found - tool not moved"));
  servo_output_write_tool(robot.toolMinDeg + (robot.toolMaxDeg - robot.toolMinDeg) * pos / 100.0f);
  comm_send(F("Tool moved"));
}

void cmd_tool_state(const char* line) {
  bool on;
  if (strstr_P(line, PSTR("(HIGH)")))      on = true;
  else if (strstr_P(line, PSTR("(LOW)")))  on = false;
  else {
    comm_send(F("ERROR: expected Tool_state (HIGH) or (LOW)"));
    return;
  }
  write_relay(on);
  comm_send(on ? F("Tool state HIGH") : F("Tool state LOW"));  // read by the Studio
}

void cmd_io_digital_write(const char* line) {
  long pin = value_after(line, 'P', -1);
  long state = value_after(line, 'S', -1);
  if (pin < 0 || (state != 0 && state != 1)) {
    comm_send(F("ERROR: expected IO_digitalWrite P<pin>S<0|1>"));
    return;
  }
  if (pin == I2C_SDA_PIN || pin == I2C_SCL_PIN) {
    comm_send(F("ERROR: that pin is used by the I2C servo driver"));
    return;
  }
  pinMode(pin, OUTPUT);
  digitalWrite(pin, state ? HIGH : LOW);
  comm_send(state ? F("Output set HIGH") : F("Output set LOW"));
}
