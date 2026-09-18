#pragma once

#include <stdint.h>

// Coin ERM vibration motor module with an onboard MOSFET driver and flyback
// diode (3-pin GND/VCC/IN header). IN is driven directly from a spare GPIO;
// no external driver circuitry is needed. See README.md "wiring" for the
// pinout.

void haptic_init(void);

// Runs the motor for on_ms and returns with it off. Feedback patterns that
// pair a buzz with an aura flash drive the steps themselves so the two stay
// together; see the indicate_* helpers in fingerprint.c.
void haptic_buzz(uint32_t on_ms);
