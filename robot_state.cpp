#include "robot_state.h"
#include "pinout.h"

RobotState robot;

// DH parameters of the ARM (theta, alpha, d, a), replaced by Set_dh_par on connect
static const float DEFAULT_DH[4][4] = {
  {0.0f, 90.0f, 114.1f, 0.0f},
  {90.0f, 0.0f, 0.0f, 148.0f},
  {-90.0f, 0.0f, 0.0f, 113.6f},
  {0.0f, 0.0f, 0.0f, 98.8f},
};

static void default_servo(JointConfig& j, uint8_t channel) {
  j.minDeg = -90.0f;
  j.maxDeg = 90.0f;
  j.speedDegS = DEFAULT_JOINT_SPEED_DEG_S;
  j.accelDegS2 = DEFAULT_JOINT_ACCEL_DEG_S2;
  j.zeroDeg = SERVO_ZERO_DEG;
  j.dir = 1;
  j.pulseMinUs = SERVO_PULSE_MIN_US;
  j.pulseMaxUs = SERVO_PULSE_MAX_US;
  j.rangeDeg = SERVO_RANGE_DEG;
  j.channel = channel;
  j.homeOrder = 0;
  j.homeDeg = 0.0f;
}

void robot_state_init() {
  memset(&robot, 0, sizeof(robot));
  robot.numJoints = DEFAULT_NUMBER_OF_JOINTS;
  robot.extraJoint = DEFAULT_EXTRA_JOINT;

  for (uint8_t i = 0; i < MAX_JOINTS; i++) {
    default_servo(robot.joint[i], JOINT_SERVO_CHANNEL[i]);
    robot.joint[i].homeOrder = i + 1;
  }
  default_servo(robot.roll, WRIST_ROLL_SERVO_CHANNEL);
  robot.roll.minDeg = WRIST_ROLL_MIN_DEG;
  robot.roll.maxDeg = WRIST_ROLL_MAX_DEG;
  default_servo(robot.pitch, WRIST_PITCH_SERVO_CHANNEL);
  robot.pitch.minDeg = WRIST_PITCH_MIN_DEG;
  robot.pitch.maxDeg = WRIST_PITCH_MAX_DEG;

  memcpy(robot.dh, DEFAULT_DH, sizeof(DEFAULT_DH));
  robot.toolChannel = TOOL_SERVO_CHANNEL;
  robot.toolMinDeg = TOOL_SERVO_MIN_DEG;
  robot.toolMaxDeg = TOOL_SERVO_MAX_DEG;
}
