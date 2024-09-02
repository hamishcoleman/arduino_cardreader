/*
 * Copyright 2024 Hamish Coleman
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * A small collection of useful tools for handling bytes and buffers.
 * It is intended to be generic enough to be reusable in other projects
 */

#include <stdint.h>
#include <HardwareSerial.h>

uint32_t buf_be2hl(uint8_t *buf) {
    uint32_t result;

    result = buf[0];
    result <<= 8;
    result |= buf[1];
    result <<= 8;
    result |= buf[2];
    result <<= 8;
    result |= buf[3];
    return result;
}

uint32_t buf_be2h24(uint8_t *buf) {
    uint32_t result;
    result = *buf++;
    result <<= 8;
    result |= *buf++;
    result <<= 8;
    result |= *buf++;
    return result;
}

uint32_t buf_le2hl(uint8_t *buf) {
    uint32_t result;

    result = buf[3];
    result <<= 8;
    result |= buf[2];
    result <<= 8;
    result |= buf[1];
    result <<= 8;
    result |= buf[0];
    return result;
}

// If only Serial.print(xyzzy, HEX) did leading zeros
// (The lack of leading zeros has been raised before and they do not appear
// to be interested in addressing it)
void hexdump(uint8_t *buf, uint8_t size) {
    char digits[] = "0123456789ABCDEF";
    while(size) {
        char ch = *buf;
        char hi = (ch >> 4) & 0x0f;
        char lo = ch & 0x0f;
        Serial.print(digits[hi]);
        Serial.print(digits[lo]);

        buf++;
        size--;
    }
}

void serial_intzeropad(uint32_t i, uint8_t zeropad) {
    uint8_t digits = 0;
    uint32_t acc = i;
    while(acc) {
        acc /= 10;
        digits ++;
    }
    while(digits < zeropad) {
        Serial.print('0');
        digits ++;
    }
    Serial.print(i);
}

