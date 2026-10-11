#include "command_parser.h"
#include <stdlib.h>

static char marker(uint8_t index) {
  return index < 26 ? char('A' + index) : char('a' + index - 26);
}

// Values only start after the command name, so letters inside the name
// ("Set_dh_par" contains an 'S') are never taken for markers.
static const char* values_start(const char* line) {
  const char* p = strchr(line, ' ');
  return p ? p + 1 : nullptr;
}

uint8_t parse_values(const char* line, float* out, uint8_t maxCount) {
  const char* p = values_start(line);
  if (!p) return 0;

  for (uint8_t i = 0; i < maxCount && i < 52; i++) {
    while (*p == ' ') p++;
    if (*p != marker(i)) return i;
    p++;

    // copy one number: sign, digits, decimals and an optional exponent
    char num[24];
    uint8_t n = 0;
    bool digits = false;
    if (*p == '-' || *p == '+') num[n++] = *p++;
    while (n < sizeof(num) - 1 && (isdigit(*p) || *p == '.')) {
      digits |= isdigit(*p) != 0;
      num[n++] = *p++;
    }
    // an 'e' is an exponent only if it is not the next marker ("D50E50" = D:50, E:50)
    char next = marker(i + 1);
    if (digits && (*p == 'e' || *p == 'E') && *p != next &&
        (isdigit(p[1]) || ((p[1] == '-' || p[1] == '+') && isdigit(p[2])))) {
      num[n++] = *p++;
      if (*p == '-' || *p == '+') num[n++] = *p++;
      while (n < sizeof(num) - 1 && isdigit(*p)) num[n++] = *p++;
    }
    if (!digits) return i;
    num[n] = '\0';
    out[i] = (float)strtod(num, nullptr);
  }
  return maxCount;
}

bool cmd_is(const char* line, PGM_P name) {
  size_t len = strlen_P(name);
  return strncmp_P(line, name, len) == 0 && (line[len] == ' ' || line[len] == '\0');
}

long value_after(const char* line, char key, long fallback) {
  const char* p = values_start(line);
  if (!p) return fallback;
  p = strchr(p, key);
  if (!p || !(isdigit(p[1]) || p[1] == '-')) return fallback;
  return atol(p + 1);
}

void append_float(char* buf, size_t size, float value) {
  char tmp[16];
  dtostrf(value, 1, 2, tmp);
  strncat(buf, tmp, size - strlen(buf) - 1);
}
