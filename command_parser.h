#pragma once
#include <Arduino.h>

// Commands from MiKoBots Studio look like "<Name> A<value>B<value>C<value>...",
// the value markers run A..Z then a..z.

// Parses the values after the command name, in marker order, into out[].
// Returns how many consecutive markers were found (stops at the first missing one).
uint8_t parse_values(const char* line, float* out, uint8_t maxCount);

// True when the first word of line equals name (name is a PROGMEM string, use PSTR()).
bool cmd_is(const char* line, PGM_P name);

// Integer that follows the first occurrence of `key` after the command name, or fallback.
long value_after(const char* line, char key, long fallback);

// Appends a float with 2 decimals to buf (works on AVR, where printf has no %f).
void append_float(char* buf, size_t size, float value);
