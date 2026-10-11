#pragma once
#include <Arduino.h>

// Tool and IO commands.

void tool_io_begin();
void cmd_tool_move_to(const char* line);     // "Tool_move_to pos(0..100)": gripper servo
void cmd_tool_state(const char* line);       // "Tool_state (HIGH|LOW)": relay / vacuum
void cmd_io_digital_write(const char* line); // "IO_digitalWrite P<pin>S<0|1>"
