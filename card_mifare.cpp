/*
 * Copyright 2024 Hamish Coleman
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * functions to decode mifare cards
 */

#include <Adafruit_PN532.h>
#include <Arduino.h>

#include "arduino_cardreader.h"
#include "byteops.h"
#include "hexdump.h"
#include "packets.h"

uint8_t mifare_read(Adafruit_PN532 nfc, uint8_t page, uint8_t * buf, uint8_t buflen) {
    uint8_t cmd[2];
    cmd[0] = MIFARE_CMD_READ;
    cmd[1] = page;

    // FIXME: set private nfc._inListedTag == tg;
    if (!nfc.inDataExchange(cmd,2,buf,&buflen)) {
        return 0;
    }
    return buflen;
}

static void decode_hsl(Adafruit_PN532 nfc, uint8_t *uid, uint8_t *page4) {
    hexdump(Serial, &page4[1], 5);
    uint32_t u1 = buf_be2h24(&uid[1]);
    uint32_t u2 = buf_be2h24(&uid[4]);
    serial_intzeropad((u1^u2)&0x7fffff, 7);
    Serial.print((page4[6]>>4), HEX);
}

static void decode_troika(Adafruit_PN532 nfc, uint8_t *page4) {
    uint32_t s = (buf_be2hl(&page4[0]) << 20) | (buf_be2hl(&page4[4]) >> 12);
    Serial.print(s);
}

static void decode_mifare7(Adafruit_PN532 nfc, uint8_t *uid) {
    uint8_t page4[16];

    if (mifare_read(nfc, 4, page4, sizeof(page4))!=sizeof(page4)) {
        return;
    }

    if (output_flags & OUTPUT_RAWALL) {
        packet_start();
        Serial.print("page[4..7]=");
        hexdump(Serial, page4, sizeof(page4));
        packet_end();
    }

    if (buf_be2h24(&page4[1]) == 0x924621) {
        packet_start();
        Serial.print("serial=hsl/");
        decode_hsl(nfc, uid, page4);
        packet_end();
        return;
    }

    if ((page4[0]==0x45) && (page4[1]&0xc0 == 0xc0)) {
        packet_start();
        Serial.print("serial=troika/");
        decode_troika(nfc, page4);
        packet_end();
        return;
    }

}

void decode_mifare(Adafruit_PN532 nfc, uint8_t *uid, uint8_t uidlen) {
    if (uidlen == 7) {
        decode_mifare7(nfc, uid);
        return;
    }
}

