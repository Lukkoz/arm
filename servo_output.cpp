#include "servo_output.h"
#include <Wire.h>
#include "pinout.h"
#include "pca9685.h"
#include "robot_state.h"
#include "communication.h"

// Every PCA9685 also answers at its ALLCALL address, so it never counts as a separate device.
static const uint8_t PCA9685_ALLCALL_ADDRESS = 0x70;
static const uint8_t MAX_LISTED_DEVICES = 8;

static Pca9685 pca;
static bool present = false;
static bool outputsOn = false;
static uint8_t driverAddress = 0;              // 0 = not found
static bool scanned = false;                   // configured address did not answer, bus was scanned
static uint8_t foundCount = 0;
static uint8_t found[MAX_LISTED_DEVICES];

static bool i2c_answers(uint8_t address) {
  Wire.beginTransmission(address);
  return Wire.endTransmission() == 0;
}

// Pulls a line low or lets the pull-up take it high (open drain, never driven high).
static void line_low(uint8_t pin) {
#if defined(CONTROLLER_ESP32)
  pinMode(pin, OUTPUT_OPEN_DRAIN);
#else
  digitalWrite(pin, LOW);               // pull-up off first, so the pin is never driven high
  pinMode(pin, OUTPUT);
#endif
  digitalWrite(pin, LOW);
}

static void line_release(uint8_t pin) {
  pinMode(pin, INPUT_PULLUP);
}

// I2C bus recovery: a reset of the controller in the middle of a transfer can leave the
// driver waiting for the rest of a byte. 9 clocks finish that byte and a STOP ends the transfer.
static void bus_recover() {
  line_release(I2C_SDA_PIN);
  line_release(I2C_SCL_PIN);
  delayMicroseconds(10);
  for (uint8_t i = 0; i < 9; i++) {
    line_low(I2C_SCL_PIN);
    delayMicroseconds(10);
    line_release(I2C_SCL_PIN);
    delayMicroseconds(10);
  }
  line_low(I2C_SDA_PIN);                // STOP: SDA rises while SCL is high
  delayMicroseconds(10);
  line_release(I2C_SDA_PIN);
  delayMicroseconds(10);
}

// PCA9685 software reset (general call 0x00, data 0x06): every PCA9685 on the bus goes back
// to its power-up state, with ALLCALL on, whatever an earlier run left in it.
static void software_reset() {
  Wire.beginTransmission(0x00);
  Wire.write(0x06);
  Wire.endTransmission();
  delay(1);
}

// Scans the bus and returns the only device on it besides ALLCALL, or 0 when there is none or several.
static uint8_t scan_for_driver() {
  uint8_t candidate = 0;
  uint8_t candidates = 0;
  scanned = true;
  foundCount = 0;
  for (uint8_t a = 0x08; a <= 0x7F; a++) {
    if (!i2c_answers(a)) continue;
    if (foundCount < MAX_LISTED_DEVICES) found[foundCount] = a;
    foundCount++;
    if (a != PCA9685_ALLCALL_ADDRESS) {
      candidate = a;
      candidates++;
    }
  }
  if (candidates == 1) return candidate;
  if (candidates == 0 && foundCount > 0) return PCA9685_ALLCALL_ADDRESS;  // only ALLCALL answers
  return 0;
}

bool servo_output_begin() {
  Wire.end();                           // also called again on CONNECT: free the pins for the recovery
  bus_recover();
#if defined(CONTROLLER_ESP32)
  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN, I2C_CLOCK_HZ);
#else
  Wire.begin();                         // AVR: fixed hardware pins
  Wire.setClock(I2C_CLOCK_HZ);
#endif
  software_reset();

  if (PCA9685_OE_PIN >= 0) {
    pinMode(PCA9685_OE_PIN, OUTPUT);
    digitalWrite(PCA9685_OE_PIN, HIGH); // outputs disabled until the first move
  }
  scanned = false;
  driverAddress = PCA9685_I2C_ADDRESS;
  if (!i2c_answers(driverAddress)) driverAddress = scan_for_driver();
  present = driverAddress != 0 &&
            pca.begin(driverAddress, PCA9685_OSC_HZ, SERVO_FREQUENCY_HZ);
  outputsOn = false;
  return present;
}

bool servo_output_present() {
  return present;
}

void servo_output_report() {
  if (!scanned) return;                 // found at the configured address, nothing to tell

  char msg[96];
  int n = snprintf(msg, sizeof msg, "I2C scan (SDA %d, SCL %d):", (int)I2C_SDA_PIN, (int)I2C_SCL_PIN);
  if (foundCount == 0) n += snprintf(msg + n, sizeof msg - n, " no devices");
  for (uint8_t i = 0; i < foundCount && i < MAX_LISTED_DEVICES && n < (int)sizeof msg; i++)
    n += snprintf(msg + n, sizeof msg - n, " 0x%02X", found[i]);
  if (foundCount > MAX_LISTED_DEVICES && n < (int)sizeof msg) snprintf(msg + n, sizeof msg - n, " ...");
  comm_send(msg);

  if (present) {
    snprintf(msg, sizeof msg, "WARNING: PCA9685 not at 0x%02X, using the device at 0x%02X",
             PCA9685_I2C_ADDRESS, driverAddress);
  } else if (foundCount == 0) {
    snprintf(msg, sizeof msg, "ERROR: PCA9685 servo driver not found on the I2C bus");
  } else {
    snprintf(msg, sizeof msg, "ERROR: PCA9685 not at 0x%02X and more than one device found, "
             "set PCA9685_I2C_ADDRESS", PCA9685_I2C_ADDRESS);
  }
  comm_send(msg);
}

static void enable_outputs() {
  if (outputsOn) return;
  if (PCA9685_OE_PIN >= 0) digitalWrite(PCA9685_OE_PIN, LOW);
  outputsOn = true;
}

// Servo angle for a joint angle: zero offset, direction, then clamped to the servo range.
static float servo_angle(const JointConfig& c, float jointDeg) {
  return constrain(c.zeroDeg + c.dir * jointDeg, 0.0f, c.rangeDeg);
}

static void write_servo(const JointConfig& c, float servoDeg) {
  float us = c.pulseMinUs + servoDeg / c.rangeDeg * (float(c.pulseMaxUs) - float(c.pulseMinUs));
  pca.writeMicroseconds(c.channel, us);
}

void servo_output_write_joints(const float* jointDeg) {
  if (!present) return;
  enable_outputs();

  for (uint8_t i = 0; i < robot.numJoints; i++) {
    float angle = jointDeg[i];
#if ELBOW_COUPLED
    if (i == 2) angle += jointDeg[1];   // elbow servo measured against the base
#endif
    write_servo(robot.joint[i], servo_angle(robot.joint[i], angle));
  }

  // wrist pitch of a 3-axis arm: level (J4 = -(J2 + J3), as MiKoBots Studio) plus the pitch
  if (robot.extraJoint && robot.numJoints >= 3 && robot.numJoints < MAX_JOINTS) {
    JointConfig wrist = robot.joint[robot.numJoints];
    wrist.channel = WRIST_PITCH_SERVO_CHANNEL;
    write_servo(wrist, servo_angle(wrist, -(jointDeg[1] + jointDeg[2]) + robot.pitchDeg));
  }
}

void servo_output_write_roll(float rollDeg) {
  if (!present) return;
  enable_outputs();
  write_servo(robot.roll, servo_angle(robot.roll, rollDeg));
}

void servo_output_write_tool(float servoDeg) {
  if (!present) return;
  enable_outputs();
  float us = SERVO_PULSE_MIN_US +
             constrain(servoDeg, 0.0f, float(SERVO_RANGE_DEG)) / SERVO_RANGE_DEG *
             (SERVO_PULSE_MAX_US - SERVO_PULSE_MIN_US);
  pca.writeMicroseconds(robot.toolChannel, us);
}
