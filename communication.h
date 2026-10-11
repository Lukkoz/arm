#pragma once
#include <Arduino.h>

// Link to MiKoBots Studio over USB serial (and BLE on ESP32).
//
// Protocol:
//  - Every command line ends with '\n'. Each command gets exactly one "END" reply,
//    which the Studio waits for before it sends the next line.
//  - "stop", "pauze" and "play" are handled immediately, also while the robot is
//    moving, and get no END: "pauze" answers "wait", "play" and "stop" answer "go".
//  - While a move runs the firmware sends "BUSY" every second, so the Studio knows
//    the robot is alive and keeps waiting for END.

void comm_begin();
void comm_poll();          // idle loop: read and execute commands
void comm_poll_motion();   // during motion: only stop / pauze / play are executed

void comm_send(const char* message);
void comm_send(const __FlashStringHelper* message);
