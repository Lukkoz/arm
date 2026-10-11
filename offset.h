#pragma once
#include <Arduino.h>

// Offset section: moves relative to the current TCP position.

void cmd_offset_j(const char* line);  // OffsetJ: X Y Z offset, joint interpolated
void cmd_offset_l(const char* line);  // OffsetL / jogL: X Y Z offset, straight line
