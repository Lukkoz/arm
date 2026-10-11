// Standalone PCA9685 test (no arm firmware code), repeats every 2 s:
// scans the I2C bus on the arm wiring, then moves every servo channel 10 degrees and back,
// one channel at a time.
#include <Wire.h>

static const int SDA_PIN = 5;                 // D4
static const int SCL_PIN = 6;                 // D5
static const uint32_t I2C_HZ = 100000;
static const uint8_t PCA_ADDRESS = 0x70;
static const uint8_t CHANNELS = 16;
static const float SERVO_CENTER_DEG = 90;
static const float SERVO_SWING_DEG = 10;
static const float PULSE_MIN_US = 500, PULSE_MAX_US = 2500, RANGE_DEG = 180;  // as the arm firmware
static const float US_PER_TICK = (121 + 1) * 1e6f / 25000000;                 // prescale 121 -> 50 Hz

static bool scan() {
  Serial.print("I2C scan D4=SDA D5=SCL 100 kHz  found:");
  int n = 0;
  for (uint8_t a = 1; a <= 0x7F; a++) {
    Wire.beginTransmission(a);
    if (Wire.endTransmission() == 0) { Serial.printf(" 0x%02X", a); n++; }
  }
  if (!n) Serial.print(" none");
  Wire.beginTransmission(PCA_ADDRESS);
  uint8_t r = Wire.endTransmission();
  Serial.printf("  | code 0x%02X=%u\n", PCA_ADDRESS, r);  // 0 ok, 2 no answer, 4 other, 5 timeout
  return r == 0;
}

static uint8_t pca_write(uint8_t reg, const uint8_t* data, uint8_t len) {
  Wire.beginTransmission(PCA_ADDRESS);
  Wire.write(reg);
  Wire.write(data, len);
  return Wire.endTransmission();
}

static uint8_t pca_write8(uint8_t reg, uint8_t value) { return pca_write(reg, &value, 1); }

static uint8_t servo_deg(uint8_t channel, float deg) {
  uint16_t ticks = lround((PULSE_MIN_US + deg / RANGE_DEG * (PULSE_MAX_US - PULSE_MIN_US)) / US_PER_TICK);
  uint8_t d[4] = {0, 0, (uint8_t)ticks, (uint8_t)(ticks >> 8)};   // ON = 0, OFF = ticks
  return pca_write(0x06 + 4 * channel, d, 4);
}

static void servo_test() {
  // sleep, prescale, totem pole, wake, restart + auto increment; ALLCALL (bit 0) stays on: the driver only answers at 0x70
  uint8_t err = pca_write8(0x00, 0x11) | pca_write8(0xFE, 121) | pca_write8(0x01, 0x04) |
                pca_write8(0x00, 0x21);
  delay(1);
  err |= pca_write8(0x00, 0xA1);
  if (err) {
    Serial.printf("servo test: PCA9685 setup failed (code %u), servos not moved\n", err);
    return;
  }
  for (uint8_t ch = 0; ch < CHANNELS; ch++) {
    uint8_t e = servo_deg(ch, SERVO_CENTER_DEG);
    delay(500);
    e |= servo_deg(ch, SERVO_CENTER_DEG + SERVO_SWING_DEG);
    delay(500);
    e |= servo_deg(ch, SERVO_CENTER_DEG);
    Serial.printf("servo test: channel %2u %.0f -> %.0f -> %.0f deg %s\n", ch, SERVO_CENTER_DEG,
                  SERVO_CENTER_DEG + SERVO_SWING_DEG, SERVO_CENTER_DEG, e ? "(I2C error)" : "ok");
  }
}

// I2C bus recovery: a reset of the XIAO in the middle of a transfer can leave the driver
// waiting for the rest of a byte. 9 clocks finish it, a STOP ends the transfer.
static void bus_recover() {
  pinMode(SDA_PIN, INPUT_PULLUP);
  pinMode(SCL_PIN, OUTPUT_OPEN_DRAIN);
  digitalWrite(SCL_PIN, HIGH);
  delayMicroseconds(10);
  int sdaBefore = digitalRead(SDA_PIN);
  for (int i = 0; i < 9; i++) {
    digitalWrite(SCL_PIN, LOW);  delayMicroseconds(10);
    digitalWrite(SCL_PIN, HIGH); delayMicroseconds(10);
  }
  pinMode(SDA_PIN, OUTPUT_OPEN_DRAIN);      // STOP: SDA rises while SCL is high
  digitalWrite(SDA_PIN, LOW);  delayMicroseconds(10);
  digitalWrite(SDA_PIN, HIGH); delayMicroseconds(10);
  pinMode(SDA_PIN, INPUT_PULLUP);
  Serial.printf("bus recovery: SDA was %s before, %s after\n", sdaBefore ? "high" : "LOW (held)",
                digitalRead(SDA_PIN) ? "high" : "LOW (still held)");
}

static void software_reset() {               // PCA9685 SWRST: general call 0x00, data 0x06
  Wire.beginTransmission(0x00);
  Wire.write(0x06);
  Serial.printf("software reset (general call): code %u\n", Wire.endTransmission());
}

static void read_registers() {
  const uint8_t regs[] = {0x00, 0x01, 0xFE};          // MODE1, MODE2, PRESCALE
  const char* names[] = {"MODE1", "MODE2", "PRESCALE"};
  for (int i = 0; i < 3; i++) {
    Wire.beginTransmission(PCA_ADDRESS);
    Wire.write(regs[i]);
    uint8_t w = Wire.endTransmission(false);           // repeated start, then read
    uint8_t n = Wire.requestFrom(PCA_ADDRESS, (uint8_t)1);
    int v = n ? Wire.read() : -1;
    Serial.printf("read %-8s: select code %u, got %u byte%s", names[i], w, n, n == 1 ? "" : "s");
    if (v >= 0) Serial.printf(" = 0x%02X", v);
    Serial.println();
  }
  uint8_t c1 = pca_write8(0x00, 0x11);
  uint8_t c2 = pca_write8(0xFE, 121);
  Serial.printf("write MODE1=0x11: code %u, write PRESCALE=121: code %u\n", c1, c2);
}

void setup() {
  Serial.begin(115200);
  delay(1500);
}

void loop() {
  Serial.println("----");
  Wire.end();
  bus_recover();
  Wire.begin(SDA_PIN, SCL_PIN, I2C_HZ);   // fresh start each round, as the scanner that found the driver
  software_reset();
  delay(1);
  if (scan()) { read_registers(); servo_test(); }
  else Serial.println("servo test: driver not found, servos not moved");
  Wire.end();
  delay(2000);
}
