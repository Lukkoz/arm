#include "servo_movement.h"
#include "command_parser.h"
#include "communication.h"
#include "kinematics.h"
#include "robot_state.h"
#include "servo_output.h"

static const char AXIS_NAME[NUM_AXES] = {'X', 'Y', 'Z', 'y', 'p', 'r'};
static const uint32_t KEEP_ALIVE_MS = 1000;   // "BUSY" while a move runs, so the Studio keeps waiting

// ---------------------------------------------------------------------
//  Helpers
// ---------------------------------------------------------------------
static bool joints_within_limits(const float* j) {
  for (uint8_t i = 0; i < robot.numJoints; i++) {
    const JointConfig& c = robot.joint[i];
    if (isnan(j[i]) || j[i] < c.minDeg || j[i] > c.maxDeg) {
      char msg[72], value[12];
      dtostrf(j[i], 1, 1, value);
      snprintf_P(msg, sizeof(msg), PSTR("ERROR: J%u = %s is outside its limits - move refused"),
                 i + 1, value);
      comm_send(msg);
      return false;
    }
  }
  return true;
}

// Waits for the next interpolation step while listening for stop / pauze / play.
// Returns false when the move has to stop.
static bool wait_next_step(uint32_t& nextStep, uint32_t& lastKeepAlive) {
  for (;;) {
    comm_poll_motion();
    if (robot.stopRequested) return false;
    if (millis() - lastKeepAlive >= KEEP_ALIVE_MS) {
      lastKeepAlive = millis();
      comm_send(F("BUSY"));
    }
    if (robot.paused) {
      nextStep = millis() + SERVO_UPDATE_MS;   // the profile clock stops during a pause
    } else if ((int32_t)(millis() - nextStep) >= 0) {
      nextStep += SERVO_UPDATE_MS;
      return true;
    }
    delay(1);
  }
}

static void warn_if_no_driver() {
  if (!servo_output_present())
    comm_send(F("WARNING: PCA9685 not found - position updated, servos not moved"));
}

static float clamp_pct(float pct) {
  return (isnan(pct) || pct <= 0) ? DEFAULT_SPEED_PCT : min(pct, 100.0f);
}

// ---------------------------------------------------------------------
//  Motion profile
// ---------------------------------------------------------------------
// Trapezoidal profile over the path fraction 0..1: accelerate for `ramp` seconds,
// cruise, decelerate for `ramp` seconds. ramp == total / 2 is a triangle profile.
struct Profile {
  float total;   // s
  float ramp;    // s
};

// Fastest profile for distance d with speed limit v and acceleration limit a.
static Profile fastest_profile(float d, float v, float a) {
  if (d <= 0 || v <= 0 || a <= 0) return {0, 0};
  float ramp = v / a;
  if (d < v * ramp) {                  // too short to reach full speed: triangle
    ramp = sqrt(d / a);
    return {2 * ramp, ramp};
  }
  return {d / v + ramp, ramp};
}

// Slows the profile down until distance d stays within v and a.
static void fit_profile(Profile& p, float d, float v, float a) {
  if (p.total <= 0 || d <= 0) return;
  float peakSpeed = d / (p.total - p.ramp);        // path speed at cruise
  float peakAccel = peakSpeed / p.ramp;
  float k = max(1.0f, max(peakSpeed / v, sqrt(peakAccel / a)));
  p.total *= k;                                     // k times slower: speed / k, acceleration / k^2
  p.ramp *= k;
}

// Path fraction (0..1) reached at time t.
static float profile_fraction(const Profile& p, float t) {
  if (p.total <= 0 || t >= p.total) return 1.0f;
  float cruise = 1.0f / (p.total - p.ramp);        // path speed at cruise
  float accel = cruise / p.ramp;
  if (t < p.ramp) return 0.5f * accel * t * t;
  if (t < p.total - p.ramp) return 0.5f * accel * p.ramp * p.ramp + cruise * (t - p.ramp);
  float left = p.total - t;
  return 1.0f - 0.5f * accel * left * left;
}

// Runs the profile and calls apply(fraction) every SERVO_UPDATE_MS.
// apply returns false to abort. Returns true when the profile completed.
template <typename Apply>
static bool run_profile(const Profile& p, Apply apply) {
  uint32_t steps = max(1UL, (uint32_t)ceil(p.total * 1000.0f / SERVO_UPDATE_MS));
  uint32_t nextStep = millis();
  uint32_t lastKeepAlive = millis();
  for (uint32_t k = 1; k <= steps; k++) {
    if (!wait_next_step(nextStep, lastKeepAlive)) {
      comm_send(F("Move stopped"));
      return false;
    }
    float t = min(p.total, k * (SERVO_UPDATE_MS / 1000.0f));
    if (!apply(k == steps ? 1.0f : profile_fraction(p, t))) return false;
  }
  return true;
}

// ---------------------------------------------------------------------
//  Motion engine
// ---------------------------------------------------------------------
bool motion_move_joints(const float* targetDeg, float speedPct, float accelPct) {
  if (!joints_within_limits(targetDeg)) return false;
  warn_if_no_driver();

  uint8_t n = robot.numJoints;
  float start[MAX_JOINTS];
  memcpy(start, robot.jointDeg, sizeof(start));
  float sp = clamp_pct(speedPct) / 100.0f, ap = clamp_pct(accelPct) / 100.0f;

  // the joint that needs the longest time sets the profile, the others follow it
  Profile p = {0, 0};
  for (uint8_t i = 0; i < n; i++) {
    Profile pi = fastest_profile(fabs(targetDeg[i] - start[i]), robot.joint[i].speedDegS * sp,
                                 robot.joint[i].accelDegS2 * ap);
    if (pi.total > p.total) p = pi;
  }
  for (uint8_t i = 0; i < n; i++)
    fit_profile(p, fabs(targetDeg[i] - start[i]), robot.joint[i].speedDegS * sp,
                robot.joint[i].accelDegS2 * ap);

  bool done = run_profile(p, [&](float f) {
    float now[MAX_JOINTS];
    for (uint8_t i = 0; i < n; i++) now[i] = start[i] + (targetDeg[i] - start[i]) * f;
    servo_output_write_joints(now);
    memcpy(robot.jointDeg, now, sizeof(now));
    return true;
  });
  kin_update_position();
  return done;
}

bool motion_move_linear(const float* targetXyz, float speedPct, float accelPct) {
  if (!require_kinematics()) return false;

  float joints[MAX_JOINTS];
  memcpy(joints, robot.jointDeg, sizeof(joints));
  if (!kin_inverse(targetXyz, joints)) {
    comm_send(F("ERROR: target position is out of reach - move refused"));
    return false;
  }
  if (!joints_within_limits(joints)) return false;
  warn_if_no_driver();

  float start[3] = {robot.axis[0], robot.axis[1], robot.axis[2]};
  float dist = sqrt(sq(targetXyz[0] - start[0]) + sq(targetXyz[1] - start[1]) +
                    sq(targetXyz[2] - start[2]));
  Profile p = fastest_profile(dist, LINEAR_SPEED_MM_S * clamp_pct(speedPct) / 100.0f,
                              LINEAR_ACCEL_MM_S2 * clamp_pct(accelPct) / 100.0f);

  bool done = run_profile(p, [&](float f) {
    float pos[3];
    for (uint8_t a = 0; a < 3; a++) pos[a] = start[a] + (targetXyz[a] - start[a]) * f;
    if (!kin_inverse(pos, joints) || !joints_within_limits(joints)) {
      comm_send(F("ERROR: straight line leaves the work area - move stopped"));
      return false;
    }
    servo_output_write_joints(joints);
    memcpy(robot.jointDeg, joints, sizeof(joints));
    return true;
  });
  kin_update_position();
  return done;
}

// Moves one wrist servo (roll or pitch) to targetDeg. `value` is its commanded angle,
// `write` sends it to the servo.
template <typename Write>
static bool move_wrist(const JointConfig& c, float& value, const __FlashStringHelper* name,
                       float targetDeg, float speedPct, float accelPct, Write write) {
  if (isnan(targetDeg) || targetDeg < c.minDeg || targetDeg > c.maxDeg) {
    char msg[72], label[12], number[12];
    strncpy_P(label, (PGM_P)name, sizeof(label) - 1);
    label[sizeof(label) - 1] = '\0';
    dtostrf(targetDeg, 1, 1, number);
    snprintf_P(msg, sizeof(msg), PSTR("ERROR: %s %s is outside its limits - move refused"),
               label, number);
    comm_send(msg);
    return false;
  }
  warn_if_no_driver();

  float start = value;
  Profile p = fastest_profile(fabs(targetDeg - start), c.speedDegS * clamp_pct(speedPct) / 100.0f,
                              c.accelDegS2 * clamp_pct(accelPct) / 100.0f);
  return run_profile(p, [&](float f) {
    value = start + (targetDeg - start) * f;
    write();
    return true;
  });
}

bool motion_move_roll(float targetDeg, float speedPct, float accelPct) {
  return move_wrist(robot.roll, robot.rollDeg, F("wrist roll"), targetDeg, speedPct, accelPct,
                    [] { servo_output_write_roll(robot.rollDeg); });
}

bool motion_move_pitch(float targetDeg, float speedPct, float accelPct) {
  if (!robot.extraJoint) {
    comm_send(F("ERROR: wrist pitch needs the extra joint (Set_extra_joint 1)"));
    return false;
  }
  bool done = move_wrist(robot.pitch, robot.pitchDeg, F("wrist pitch"), targetDeg, speedPct,
                         accelPct, [] { servo_output_write_joints(robot.jointDeg); });
  kin_update_position();                 // the TCP moves with the gripper
  return done;
}

// ---------------------------------------------------------------------
//  Command helpers
// ---------------------------------------------------------------------
bool parse_motion(const char* line, float* values, float& speedPct, float& accelPct) {
  float v[MAX_JOINTS + 2];
  uint8_t n = robot.numJoints;
  uint8_t got = parse_values(line, v, n + 2);
  if (got < n) {
    char msg[64];
    snprintf_P(msg, sizeof(msg), PSTR("ERROR: expected %u joint/axis values, got %u"), n, got);
    comm_send(msg);
    return false;
  }
  memcpy(values, v, n * sizeof(float));
  speedPct = got > n ? v[n] : DEFAULT_SPEED_PCT;
  accelPct = got > n + 1 ? v[n + 1] : DEFAULT_SPEED_PCT;
  return true;
}

bool require_kinematics() {
  if (kin_supported()) return true;
  comm_send(F("ERROR: Cartesian moves are only implemented for 3-axis robots"));
  return false;
}

void send_position() {
  char buf[200];
  strcpy_P(buf, PSTR("POS: "));
  for (uint8_t i = 0; i < robot.numJoints; i++) {
    char name[4] = {AXIS_NAME[i], ' ', '\0'};
    strncat(buf, name, sizeof(buf) - strlen(buf) - 1);
    append_float(buf, sizeof(buf), robot.axis[i]);
    strncat(buf, " ", sizeof(buf) - strlen(buf) - 1);
  }
  for (uint8_t i = 0; i < robot.numJoints; i++) {
    char name[6];
    snprintf_P(name, sizeof(name), PSTR("J%u "), i + 1);
    strncat(buf, name, sizeof(buf) - strlen(buf) - 1);
    append_float(buf, sizeof(buf), robot.jointDeg[i]);
    strncat(buf, " ", sizeof(buf) - strlen(buf) - 1);
  }
  comm_send(buf);
  // the original firmware also reported this line; kept for compatibility
  memmove(buf + 9, buf + 4, strlen(buf + 4) + 1);   // make room: "POS:" -> "PSJstart:"
  memcpy(buf, "PSJstart:", 9);
  comm_send(buf);
}

// ---------------------------------------------------------------------
//  Commands
// ---------------------------------------------------------------------
void cmd_move_joint(const char* line) {
  float target[MAX_JOINTS], speed, accel;
  if (parse_motion(line, target, speed, accel)) motion_move_joints(target, speed, accel);
  send_position();
}

void cmd_jog_joint(const char* line) {
  float delta[MAX_JOINTS], speed, accel;
  if (parse_motion(line, delta, speed, accel)) {
    float target[MAX_JOINTS];
    for (uint8_t i = 0; i < robot.numJoints; i++) target[i] = robot.jointDeg[i] + delta[i];
    motion_move_joints(target, speed, accel);
  }
  send_position();
}

void cmd_move_j(const char* line) {
  float xyz[MAX_JOINTS], speed, accel;
  if (parse_motion(line, xyz, speed, accel) && require_kinematics()) {
    float target[MAX_JOINTS];
    memcpy(target, robot.jointDeg, sizeof(target));
    if (kin_inverse(xyz, target)) motion_move_joints(target, speed, accel);
    else comm_send(F("ERROR: target position is out of reach - move refused"));
  }
  send_position();
}

void cmd_move_l(const char* line) {
  float xyz[MAX_JOINTS], speed, accel;
  if (parse_motion(line, xyz, speed, accel)) motion_move_linear(xyz, speed, accel);
  send_position();
}

void cmd_wrist_roll(const char* line) {
  float v[3];
  uint8_t got = parse_values(line, v, 3);
  if (got < 1) {
    comm_send(F("ERROR: expected WristRoll A<deg> [B<speed %>] [C<accel %>]"));
    return;
  }
  motion_move_roll(v[0], got > 1 ? v[1] : DEFAULT_SPEED_PCT, got > 2 ? v[2] : DEFAULT_SPEED_PCT);
  char msg[32];
  strcpy_P(msg, PSTR("ROLL: "));
  append_float(msg, sizeof(msg), robot.rollDeg);
  comm_send(msg);
}

void cmd_wrist_pitch(const char* line) {
  float v[3];
  uint8_t got = parse_values(line, v, 3);
  if (got < 1) {
    comm_send(F("ERROR: expected WristPitch A<deg> [B<speed %>] [C<accel %>]"));
    return;
  }
  motion_move_pitch(v[0], got > 1 ? v[1] : DEFAULT_SPEED_PCT, got > 2 ? v[2] : DEFAULT_SPEED_PCT);
  char msg[32];
  strcpy_P(msg, PSTR("PITCH: "));
  append_float(msg, sizeof(msg), robot.pitchDeg);
  comm_send(msg);
  send_position();
}
