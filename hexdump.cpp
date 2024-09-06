/*
 * Copyright 2024 Hamish Coleman
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Just implment a simple and sane hexdump
 */

#include <stdint.h>
#include <Print.h>

// If only Serial.print(xyzzy, HEX) did leading zeros
// (The lack of leading zeros has been raised before and they do not appear
// to be interested in addressing it)
void hexdump(Print& p, uint8_t *buf, uint8_t size) {
    while(size) {
        uint8_t ch = *buf;
        if (ch < 0x10) {
            p.print('0');
        }
        p.print(ch, HEX);

        buf++;
        size--;
    }
}
