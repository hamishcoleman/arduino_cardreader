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
#include "card.h"
#include "hexdump.h"
#include "packets.h"

uint8_t mifare_read(Adafruit_PN532& nfc, uint8_t page, uint8_t * buf, uint8_t buflen) {
    uint8_t cmd[2];
    cmd[0] = MIFARE_CMD_READ;
    cmd[1] = page;

    // FIXME: set private nfc._inListedTag == tg;
    if (!nfc.inDataExchange(cmd,2,buf,&buflen)) {
        return 0;
    }
    return buflen;
}

static void decode_hsl(Adafruit_PN532& nfc, Card& card, uint8_t *page4) {
    uint32_t u1 = buf_be2h24(&card.uid[1]);
    uint32_t u2 = buf_be2h24(&card.uid[4]);

    snprintf(
        card.info,
        sizeof(card.info),
        "%02x%02x%02x%02x%02x%07lu%x",
        page4[1],page4[2],page4[3],page4[4],page4[5],
        (u1^u2)&0x7fffff,
        (page4[6]>>4)
    );
    card.info_type = INFO_TYPE_SERIAL_HSL;
}

static void decode_troika(Adafruit_PN532& nfc, uint8_t *page4) {
    uint32_t s = (buf_be2hl(&page4[0]) << 20) | (buf_be2hl(&page4[4]) >> 12);
    Serial.print(s);
}

static void decode_mifare7(Adafruit_PN532& nfc, Card& card) {
    // TODO: update Card class to cache read pages
    uint8_t page4[16];

    if (mifare_read(nfc, 4, page4, sizeof(page4))!=sizeof(page4)) {
        return;
    }

    if (output_flags & OUTPUT_RAWALL) {
        packet_start(Serial);
        Serial.print("page[4..7]=");
        hexdump(Serial, page4, sizeof(page4));
        packet_end(Serial);
    }

    if (buf_be2h24(&page4[1]) == 0x924621) {
        // Flush any existing info
        card.print_info_msg(Serial);
        decode_hsl(nfc, card, page4);
        return;
    }

    if ((page4[0]==0x45) && (page4[1]&0xc0 == 0xc0)) {
        packet_start(Serial);
        Serial.print("serial=troika/");
        decode_troika(nfc, page4);
        packet_end(Serial);
        return;
    }

}

void decode_mifare(Adafruit_PN532& nfc, Card& card) {
    if (card.uid_len == 7) {
        decode_mifare7(nfc, card);
        return;
    }
}

