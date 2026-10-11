#pragma once
#include <Arduino.h>

// Homing section.
// Servos have no position feedback: homing moves the joints one by one, in the
// "home order" from the Studio settings, to their home angle. The first move after
// power-up starts from an unknown position, so that joint jumps to its home angle.
void cmd_home(const char* line);
