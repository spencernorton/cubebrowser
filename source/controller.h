/* One coherent controller owns buttons and both sticks; neutral arms takeover. */
#pragma once
#include <stdbool.h>
#include <stdint.h>
typedef struct { bool connected; uint32_t buttons; int x, y, scroll; } ControllerSample;
typedef struct { int owner; bool armed[4]; } ControllerOwner;
int controller_choose(ControllerOwner *state, const ControllerSample samples[4], int deadzone);
