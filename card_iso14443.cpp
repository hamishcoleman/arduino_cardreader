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
#include "packets.h"

bool iso14443a_select_app(Adafruit_PN532 nfc, uint8_t tg, uint32_t app) {
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

uint8_t iso14443a_read_file(Adafruit_PN532 nfc, uint8_t tg, uint8_t file, uint8_t offset, uint8_t size, uint8_t *buf, uint8_t buflen) {
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

static void do_iso14443a_clipper(Adafruit_PN532 nfc, uint8_t tg) {
    Serial.print("clipper/");

    if (!iso14443a_select_app(nfc, tg, 0x9011f2)) {
        return;
    }

    uint8_t buf[8];
    if (iso14443a_read_file(nfc, tg,8,1,4,buf,sizeof(buf)) != 5) {
        return;
    }

    Serial.print(buf_be2hl(&buf[1]));
}

static void do_iso14443a_opal(Adafruit_PN532 nfc, uint8_t tg) {
    Serial.print("opal/");

    if (!iso14443a_select_app(nfc, tg, 0x314553)) {
        return;
    }

    uint8_t buf[6];
    if (iso14443a_read_file(nfc, tg,7,0,5,buf,sizeof(buf)) != 6) {
        return;
    }

    // TODO: what if the uint32 is >999999999 ??
    Serial.print("308522");
    serial_intzeropad(buf_le2hl(&buf[1]),9);

    Serial.print(buf[5] & 0xf);
}

static void do_iso14443a_myki(Adafruit_PN532 nfc, uint8_t tg) {
    Serial.print("myki/");

    if (!iso14443a_select_app(nfc, tg, 0x11F2)) {
        return;
    }

    uint8_t buf[10];
    if (iso14443a_read_file(nfc, tg,0xf,0,8,buf,sizeof(buf)) != 9) {
        return;
    }

    // TODO: what if the second uint32 is >99999999 ??
    serial_intzeropad(buf_le2hl(&buf[1]),6);
    serial_intzeropad(buf_le2hl(&buf[5]),8);

    // TODO: calculating the Luhn check digit would be annoying
    Serial.print('x');
}

uint8_t do_iso14443a_apps(Adafruit_PN532 nfc, uint8_t tg, uint8_t *res, uint8_t reslen) {
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

void decode_iso14443a(Adafruit_PN532 nfc, uint8_t tg) {
    uint8_t res[10];

    uint8_t reslen = do_iso14443a_apps(nfc, tg, res, sizeof(res));
    if (output_flags & OUTPUT_RAWALL) {
        packet_start();
        Serial.print("apps=");
        hexdump(res, reslen);
        packet_end();
    }

    uint8_t pos = 1;
    while(pos < reslen) {
        uint32_t app = buf_be2h24(&res[pos]);
        pos += 3;

        // TODO:
        // - we return after the first matched app ID, which might not be
        //   the correct answer.  (So far, only myki has more than one app)

        packet_start();
        Serial.print("serial=");
        switch(app) {
            case 0x11f2:
                do_iso14443a_myki(nfc, tg);
                goto end1;
            case 0x314553:
                do_iso14443a_opal(nfc, tg);
                goto end1;
            case 0x9011f2:
                do_iso14443a_clipper(nfc, tg);
                goto end1;
        }
    }

end1:
    packet_end();
}

