#include "communication.h"
#include "config.h"
#include "command_parser.h"
#include "robot_state.h"
#include "robot_setup.h"
#include "servo_movement.h"
#include "offset.h"
#include "homing.h"
#include "tool_io.h"
#include "servo_output.h"

// =====================================================================
//  Bluetooth LE (ESP32)
// =====================================================================
#if BLE_ENABLED
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

#define SERVICE_UUID_ROBOT        "a917e658-9c1a-4901-bbb8-92d54cfa2fdd"
#define CHARACTERISTIC_UUID_ROBOT "c42e42e4-8214-420c-944d-e127cc0f20ba"

#define BLE_MTU 247                    // notifications longer than MTU - 3 bytes get truncated

static BLECharacteristic* bleCharacteristic = nullptr;
static bool bleConnected = false;

// Writes from the Studio are queued here by the BLE task as they arrive and read by
// loop() like serial input, so they never share the characteristic value with the
// notifications going out.
static char bleRx[512];
static uint16_t bleRxHead = 0;
static uint16_t bleRxTail = 0;
static portMUX_TYPE bleRxMux = portMUX_INITIALIZER_UNLOCKED;

static void ble_rx_push(char c) {      // caller holds bleRxMux
  uint16_t next = (bleRxHead + 1) % sizeof(bleRx);
  if (next == bleRxTail) return;       // full: drop
  bleRx[bleRxHead] = c;
  bleRxHead = next;
}

static int ble_rx_read() {
  int c = -1;
  portENTER_CRITICAL(&bleRxMux);
  if (bleRxTail != bleRxHead) {
    c = bleRx[bleRxTail];
    bleRxTail = (bleRxTail + 1) % sizeof(bleRx);
  }
  portEXIT_CRITICAL(&bleRxMux);
  return c;
}

class RobotServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer*) override { bleConnected = true; }
  void onDisconnect(BLEServer* server) override {
    bleConnected = false;
    server->startAdvertising();
  }
};

class RobotCharacteristicCallbacks : public BLECharacteristicCallbacks {
  // Every write is one command line; the Studio sends it without '\n'.
  void onWrite(BLECharacteristic* characteristic) override {
    String value = characteristic->getValue();
    if (value.length() == 0) return;
    portENTER_CRITICAL(&bleRxMux);
    for (size_t i = 0; i < value.length(); i++) ble_rx_push(value[i]);
    if (value[value.length() - 1] != '\n') ble_rx_push('\n');
    portEXIT_CRITICAL(&bleRxMux);
  }
};

static void ble_begin() {
  BLEDevice::init(BLE_DEVICE_NAME);
  BLEDevice::setMTU(BLE_MTU);
  BLEServer* server = BLEDevice::createServer();
  server->setCallbacks(new RobotServerCallbacks());
  BLEService* service = server->createService(SERVICE_UUID_ROBOT);
  bleCharacteristic = service->createCharacteristic(
      CHARACTERISTIC_UUID_ROBOT,
      BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE |
      BLECharacteristic::PROPERTY_NOTIFY);
#if defined(CONFIG_BLUEDROID_ENABLED)
  bleCharacteristic->addDescriptor(new BLE2902());   // NimBLE adds this descriptor itself
#endif
  bleCharacteristic->setCallbacks(new RobotCharacteristicCallbacks());
  service->start();
  server->getAdvertising()->start();
}
#endif

// =====================================================================
//  Sending
// =====================================================================
void comm_send(const char* message) {
  Serial.println(message);
#if BLE_ENABLED
  if (bleConnected && bleCharacteristic) {
    bleCharacteristic->setValue((uint8_t*)message, strlen(message));
    bleCharacteristic->notify();
  }
#endif
}

void comm_send(const __FlashStringHelper* message) {
  char buf[96];
  strncpy_P(buf, (PGM_P)message, sizeof(buf) - 1);
  buf[sizeof(buf) - 1] = '\0';
  comm_send(buf);
}

// =====================================================================
//  Receiving
// =====================================================================
// rxLine collects the incoming line; a complete line is copied to cmdLine and executed,
// so control commands can still be received while that command is running.
static char rxLine[LINE_BUFFER_SIZE];
static char cmdLine[LINE_BUFFER_SIZE];
static uint16_t rxLength = 0;
static bool rxReady = false;
static bool rxOverflow = false;

static void rx_reset() {
  rxLength = 0;
  rxReady = false;
  rxOverflow = false;
}

static void rx_char(char c) {
  if (c == '\n') {
    rxLine[rxLength] = '\0';
    rxReady = true;
  } else if (c == '\r') {
    // ignore
  } else if (rxLength < LINE_BUFFER_SIZE - 1) {
    rxLine[rxLength++] = c;
  } else {
    rxOverflow = true;
  }
}

static void read_input() {
  while (!rxReady && Serial.available() > 0) rx_char(Serial.read());
#if BLE_ENABLED
  int c;
  while (!rxReady && (c = ble_rx_read()) >= 0) rx_char((char)c);
#endif
}

// stop / pauze / play: executed at once, never answered with END
static bool handle_control(const char* line) {
  if (strcmp_P(line, PSTR("stop")) == 0) {
    robot.stopRequested = true;
    robot.paused = false;
    comm_send(F("go"));                // releases a Studio that waits after "wait"
    return true;
  }
  if (strcmp_P(line, PSTR("pauze")) == 0) {
    robot.paused = true;
    comm_send(F("wait"));
    return true;
  }
  if (strcmp_P(line, PSTR("play")) == 0) {
    robot.paused = false;
    robot.stopRequested = false;
    comm_send(F("go"));
    return true;
  }
  return false;
}

static void execute(const char* line) {
  robot.stopRequested = false;

  // ---- connection
  if (cmd_is(line, PSTR("CONNECT"))) {
    comm_send(F("ROBOT_CONNECTED"));
    if (!servo_output_present()) servo_output_begin();   // look again, e.g. after fixing the wiring
    servo_output_report();
  }
  // ---- servo movement
  else if (cmd_is(line, PSTR("MoveJoint")))            cmd_move_joint(line);
  else if (cmd_is(line, PSTR("jogJ")))                 cmd_jog_joint(line);
  else if (cmd_is(line, PSTR("MoveJ")))                cmd_move_j(line);
  else if (cmd_is(line, PSTR("MoveL")))                cmd_move_l(line);
  else if (cmd_is(line, PSTR("WristRoll")))            cmd_wrist_roll(line);
  else if (cmd_is(line, PSTR("WristPitch")))           cmd_wrist_pitch(line);
  // ---- offset
  else if (cmd_is(line, PSTR("OffsetJ")))              cmd_offset_j(line);
  else if (cmd_is(line, PSTR("OffsetL")))              cmd_offset_l(line);
  else if (cmd_is(line, PSTR("jogL")))                 cmd_offset_l(line);
  // ---- homing
  else if (cmd_is(line, PSTR("Home")))                 cmd_home(line);
  // ---- tool and IO
  else if (cmd_is(line, PSTR("Tool_move_to")))         cmd_tool_move_to(line);
  else if (cmd_is(line, PSTR("Tool_state")))           cmd_tool_state(line);
  else if (cmd_is(line, PSTR("IO_digitalWrite")))      cmd_io_digital_write(line);
  // ---- setup
  else if (!setup_command(line)) {
    char msg[64];
    snprintf_P(msg, sizeof(msg), PSTR("ERROR: unknown command %.40s"), line);
    comm_send(msg);
  }

  comm_send(F("END"));                 // exactly one END per command
}

static void take_line() {
  if (rxOverflow) {
    rx_reset();
    comm_send(F("ERROR: command too long"));
    comm_send(F("END"));
    return;
  }
  if (rxLine[0] == '\0' || handle_control(rxLine)) {
    rx_reset();
    return;
  }
  memcpy(cmdLine, rxLine, rxLength + 1);
  rx_reset();
  execute(cmdLine);
}

void comm_begin() {
#if ARDUINO_USB_CDC_ON_BOOT && ARDUINO_USB_MODE
  // USB serial of the S3: when nobody reads the port (Studio on BLE) a full buffer would
  // block every message for 2 s, so drop the output instead; 1 kB covers any burst.
  Serial.setTxBufferSize(1024);
  Serial.setTxTimeoutMs(0);
#endif
  Serial.begin(SERIAL_BAUD);
#if BLE_ENABLED
  ble_begin();
#endif
}

void comm_poll() {
  read_input();
  if (rxReady) take_line();
}

void comm_poll_motion() {
  read_input();
  if (!rxReady) return;
  if (rxOverflow) {
    rx_reset();
    return;
  }
  // a normal command stays in rxLine and runs after the current one has finished
  if (handle_control(rxLine)) rx_reset();
}
