/*
 * Copyright 2024 Hamish Coleman
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * This is a timer driven, automatic LED state handler.
 * It is intended to be generic enough to be reusable in other projects
 */

#define LED_MODE_OFF    0
#define LED_MODE_BLINK1 1   // phase1
#define LED_MODE_BLINK2 2   // phase2
#define LED_MODE_ON     9

struct led_status {
    uint8_t pin;
    uint8_t mode;
    unsigned long next_state_millis;
};

void ledtimer_init();

