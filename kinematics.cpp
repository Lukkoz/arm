#include "kinematics.h"
#include "robot_state.h"

typedef float Mat4[4][4];

static void identity(Mat4 m) {
  for (uint8_t r = 0; r < 4; r++)
    for (uint8_t c = 0; c < 4; c++) m[r][c] = (r == c) ? 1.0f : 0.0f;
}

static void multiply_into(Mat4 a, const Mat4 b) {  // a = a * b
  Mat4 out;
  for (uint8_t r = 0; r < 4; r++)
    for (uint8_t c = 0; c < 4; c++) {
      float s = 0;
      for (uint8_t k = 0; k < 4; k++) s += a[r][k] * b[k][c];
      out[r][c] = s;
    }
  memcpy(a, out, sizeof(Mat4));
}

static void dh_matrix(float thetaDeg, float alphaDeg, float d, float a, Mat4 m) {
  float ct = cos(radians(thetaDeg)), st = sin(radians(thetaDeg));
  float ca = cos(radians(alphaDeg)), sa = sin(radians(alphaDeg));
  m[0][0] = ct; m[0][1] = -st * ca; m[0][2] = st * sa;  m[0][3] = a * ct;
  m[1][0] = st; m[1][1] = ct * ca;  m[1][2] = -ct * sa; m[1][3] = a * st;
  m[2][0] = 0;  m[2][1] = sa;       m[2][2] = ca;       m[2][3] = d;
  m[3][0] = 0;  m[3][1] = 0;        m[3][2] = 0;        m[3][3] = 1;
}

// Tool frame [x, y, z, rz, ry, rx] as in ForwardKinematics_3.ToolMatrix
static void tool_matrix(const float* tf, Mat4 m) {
  float cz = cos(radians(tf[3])), sz = sin(radians(tf[3]));
  float cy = cos(radians(tf[4])), sy = sin(radians(tf[4]));
  float cx = cos(radians(tf[5])), sx = sin(radians(tf[5]));
  identity(m);
  m[0][0] = cz * cy; m[0][1] = cz * sy * sx - sz * cx; m[0][2] = cz * sy * cx + sz * sx; m[0][3] = tf[0];
  m[1][0] = sz * cy; m[1][1] = sz * sy * sx + cz * cx; m[1][2] = sz * sy * cx - cz * sx; m[1][3] = tf[1];
  m[2][0] = -sy;     m[2][1] = cy * sx;                m[2][2] = cy * cx;                m[2][3] = tf[2];
}

bool kin_supported() {
  return robot.numJoints == 3;
}

bool kin_forward(const float* jointDeg, float* axisOut) {
  for (uint8_t i = 0; i < 6; i++) axisOut[i] = 0;
  if (!kin_supported()) return false;

  Mat4 T, Ti;
  identity(T);
  float theta[4];
  uint8_t rows = 3 + (robot.extraJoint ? 1 : 0);
  for (uint8_t i = 0; i < rows; i++) {
    if (i == 3) theta[i] = robot.dh[3][0] - (theta[1] + theta[2]) + robot.pitchDeg;  // level + pitch
    else        theta[i] = robot.dh[i][0] + jointDeg[i];
    dh_matrix(theta[i], robot.dh[i][1], robot.dh[i][2], robot.dh[i][3], Ti);
    multiply_into(T, Ti);
  }
  tool_matrix(robot.toolFrame, Ti);
  multiply_into(T, Ti);

  // same decomposition as convert_matrix.MatrixToXYZ
  float ry = atan2(-T[2][0], sqrt(T[2][1] * T[2][1] + T[2][2] * T[2][2]));
  axisOut[0] = T[0][3];
  axisOut[1] = T[1][3];
  axisOut[2] = T[2][3];
  axisOut[3] = degrees(atan2(T[1][0] / cos(ry), T[0][0] / cos(ry)));
  axisOut[4] = degrees(ry);
  axisOut[5] = degrees(atan2(T[2][1] / cos(ry), T[2][2] / cos(ry)));
  return true;
}

bool kin_inverse(const float* xyz, float* jointDegOut) {
  if (!kin_supported()) return false;

  // tool offsets are applied like InverseKinematics_3 in MiKoBots Studio (tool Y is vertical)
  float X = xyz[0] - robot.toolFrame[0];
  float Y = xyz[1] - robot.toolFrame[2];
  float Z = xyz[2] - robot.toolFrame[1];

  float L1 = robot.dh[0][2];
  float L2 = robot.dh[1][3];
  float L3 = robot.dh[2][3];
  float L4 = robot.extraJoint ? robot.dh[3][3] : 0.0f;
  if (L2 <= 0 || L3 <= 0) return false;

  // the gripper (length L4) points at robot.pitchDeg above the horizontal
  float p = radians(robot.pitchDeg);
  float r = sqrt(X * X + Y * Y) - L4 * cos(p);   // radial distance of the wrist pivot
  float h = Z - L1 - L4 * sin(p);                // height of the wrist pivot above the shoulder
  float D = sqrt(h * h + r * r);
  if (D < 1e-3f) return false;

  float c1 = (D * D + L2 * L2 - L3 * L3) / (2 * D * L2);
  float c3 = (D * D - L3 * L3 - L2 * L2) / (2 * L3 * L2);
  if (fabs(c1) > 1.0f || fabs(c3) > 1.0f) return false;   // out of reach

  jointDegOut[0] = degrees(atan2(Y, X));
  jointDegOut[1] = degrees(acos(c1) + atan2(h, r)) - 90.0f;
  jointDegOut[2] = 90.0f - degrees(acos(c3));
  return true;
}

void kin_update_position() {
  kin_forward(robot.jointDeg, robot.axis);
}
