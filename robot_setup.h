#pragma once
#include <Arduino.h>

// Setup section: the "Set_..." settings MiKoBots Studio sends after connecting.
// Returns false when the line is not a setup command.
bool setup_command(const char* line);
