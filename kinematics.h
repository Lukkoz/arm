#pragma once
#include <Arduino.h>

// Kinematics of a 3-axis arm with an optional level-keeping wrist (extra joint),
// using the same DH and tool frame conventions as MiKoBots Studio
// (backend/calculations/kinematics_3_axis.py), so robot and simulation agree.

bool kin_supported();                                   // only 3-axis arms for now
bool kin_forward(const float* jointDeg, float* axisOut); // TCP: X Y Z + rotation (rz ry rx)
bool kin_inverse(const float* xyz, float* jointDegOut); // false when unreachable
void kin_update_position();                             // robot.axis from robot.jointDeg
