/*
 * Copyright 2024 Hamish Coleman
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * functions to interface with ISO14443A cards
 */

#include <Adafruit_PN532.h>
#include <Arduino.h>
#include "arduino_cardreader.h"
#include "byteops.h"
#include "card.h"
#include "hexdump.h"
#include "packets.h"

bool iso14443a_select_app(Adafruit_PN532& nfc, uint8_t tg, uint32_t app) {
    uint8_t cmd[4];
    cmd[0] = 0x5a;   // Select Application
    cmd[1] = (app & 0xff0000) >> 16;
    cmd[2] = (app & 0xff00) >> 8;
    cmd[3] = (app & 0xff);
    uint8_t reslen = sizeof(cmd);

    // FIXME: set private nfc._inListedTag == tg;
    if (!nfc.inDataExchange(cmd,4,cmd,&reslen)) {
        return false;
    }
    return true;
}

uint8_t iso14443a_read_file(Adafruit_PN532& nfc, uint8_t tg, uint8_t file, uint8_t offset, uint8_t size, uint8_t *buf, uint8_t buflen) {
    uint8_t cmd[8];
    cmd[0] = 0xbd;  // Read Data
    cmd[1] = file;  // File no
    cmd[2] = offset;
    cmd[3] = 0;
    cmd[4] = 0;     // read offset high byte
    cmd[5] = size;
    cmd[6] = 0;
    cmd[7] = 0;     // read size high byte

    // FIXME: set private nfc._inListedTag == tg;
    if (!nfc.inDataExchange(cmd,8,buf,&buflen)) {
        return 0;
    }
    return buflen;
}

static void do_iso14443a_clipper(Adafruit_PN532& nfc, uint8_t tg, Card& card) {
    if (!iso14443a_select_app(nfc, tg, 0x9011f2)) {
        return;
    }

    uint8_t buf[8];
    if (iso14443a_read_file(nfc, tg,8,1,4,buf,sizeof(buf)) != 5) {
        return;
    }

    // Flush old info, before overwriting
    card.print_info_msg(Serial);
    card.set_info(
        "%9lu",
        buf_be2hl(&buf[1])
    );
    card.set_info_type(INFO_TYPE_SERIAL_CLIPPER);
}

static void do_iso14443a_opal(Adafruit_PN532& nfc, uint8_t tg, Card& card) {
    if (!iso14443a_select_app(nfc, tg, 0x314553)) {
        return;
    }

    uint8_t buf[6];
    if (iso14443a_read_file(nfc, tg,7,0,5,buf,sizeof(buf)) != 6) {
        return;
    }

    // Flush old info, before overwriting
    card.print_info_msg(Serial);
    // TODO: what if the uint32 is >999999999 ??
    card.set_info(
        "308522%09lu%i",
        buf_le2hl(&buf[1]),
        buf[5] & 0xf
    );
    card.set_info_type(INFO_TYPE_SERIAL_OPAL);
}

static void do_iso14443a_myki(Adafruit_PN532& nfc, uint8_t tg, Card& card) {
    if (!iso14443a_select_app(nfc, tg, 0x11F2)) {
        return;
    }

    uint8_t buf[10];
    if (iso14443a_read_file(nfc, tg,0xf,0,8,buf,sizeof(buf)) != 9) {
        return;
    }

    // Flush old info, before overwriting
    card.print_info_msg(Serial);
    // TODO:
    // - what if the second uint32 is >99999999 ??
    // - calculate the Luhn check digit (instead of just 'x')
    card.set_info(
        "%06lu%08lu%c",
        buf_le2hl(&buf[1]),
        buf_le2hl(&buf[5]),
        'x'
    );
    card.set_info_type(INFO_TYPE_SERIAL_MIKI);
}

uint8_t do_iso14443a_apps(Adafruit_PN532& nfc, uint8_t tg, uint8_t *res, uint8_t reslen) {
    uint8_t cmd[1];
    cmd[0] = 0x6a;   // Get Application IDs

    // TODO: set private nfc._inListedTag == tg;
    bool status = nfc.inDataExchange(cmd,1,res,&reslen);
    if (!status) {
        return 0;
    }

    // TODO:
    // We blithely assume that there are no continuation packets for this data

    return reslen;
}

void decode_iso14443a(Adafruit_PN532& nfc, uint8_t tg, Card& card) {
    uint8_t res[10];

    uint8_t reslen = do_iso14443a_apps(nfc, tg, res, sizeof(res));
    if (output_flags & OUTPUT_RAWALL) {
        packet_start(Serial);
        Serial.print("apps=");
        hexdump(Serial, res, reslen);
        packet_end(Serial);
    }

    uint8_t pos = 1;
    while(pos < reslen) {
        uint32_t app = buf_be2h24(&res[pos]);
        pos += 3;

        // TODO:
        // - we return after the first matched app ID, which might not be
        //   the correct answer.  (So far, only myki has more than one app)

        switch(app) {
            case 0x11f2:
                do_iso14443a_myki(nfc, tg, card);
                return;
            case 0x314553:
                do_iso14443a_opal(nfc, tg, card);
                return;
            case 0x9011f2:
                do_iso14443a_clipper(nfc, tg, card);
                return;
        }
    }
}

