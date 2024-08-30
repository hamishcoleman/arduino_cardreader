/*
 * Copyright 2024 Hamish Coleman
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Packetised input and output handling
 */

#include <Arduino.h>
#include "arduino_cardreader.h"
#include "ledtimer.h"

static void handle_serial_cmd(uint8_t *cmd, uint8_t len) {
    // Only trivial one char commands are implemented
    if (len != 1) {
        return;
    }

    switch (cmd[0]) {
        case 'H':
            Serial.println("Hello");
            return;
        case '0':
            led1.mode = LED_MODE_OFF;
            led2.mode = LED_MODE_OFF;
            return;
        case '1':
            led1.mode = LED_MODE_ON;
            led1.next_state_millis = millis() + 20000;
            return;
        case '2':
            led2.mode = LED_MODE_ON;
            led2.next_state_millis = millis() + 20000;
            return;
        case '3':
            led1.mode = LED_MODE_BLINK1;
            led1.next_state_millis = millis() + 20000;
            return;
        case '4':
            led2.mode = LED_MODE_BLINK1;
            led2.next_state_millis = millis() + 20000;
            return;
        case '5':
            led1.mode = LED_MODE_BLINK2;
            led1.next_state_millis = millis() + 20000;
            return;
        case '6':
            led2.mode = LED_MODE_BLINK2;
            led2.next_state_millis = millis() + 20000;
            return;
        case '7':
            led1.mode = LED_MODE_BLINK1;
            led1.next_state_millis = millis() + 20000;
            led2.mode = LED_MODE_BLINK2;
            led2.next_state_millis = millis() + 20000;
            return;
        case 'r':
            output_flags |= OUTPUT_RAWALL;
            return;
        case 'R':
            output_flags &= ~OUTPUT_RAWALL;
            return;
        case 't':
            output_flags |= OUTPUT_RAWTAG;
            return;
        case 'T':
            output_flags &= ~OUTPUT_RAWTAG;
            return;
    }
}

// Buffer to accumulate incoming message packets
uint8_t cmd[8];
uint8_t cmdpos = 0xff;

void handle_serial(uint8_t ch) {
    if (ch == '\x02') {
        // Start of frame
        cmdpos = 0;
        return;
    }
    if (ch == '\x04') {
        // End of frame
        handle_serial_cmd(cmd, cmdpos);
        cmdpos = 0xff;
        return;
    }
    if (cmdpos == 0xff) {
        // Discard bytes outside of packet frame
        return;
    }
    if (cmdpos >= sizeof(cmd)) {
        // Overflow, alert and discard until end of packet
        Serial.print('\x15');
        cmdpos = 0xff;
        return;
    }
    cmd[cmdpos++] = ch;
}
