/*
 * Copyright 2024 Hamish Coleman
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * functions to decode iso7816 cards
 */

#include <Adafruit_PN532.h>
#include <Arduino.h>

#include "byteops.h"
#include "card_iso7816.h"

#define APDU_selectByID     0
#define APDU_selectFile     1
#define APDU_readRecord     2
#define APDU_readPSE        3
#define APDU_getBalance     4
#define APDU_readBinary     5

static uint8_t * apdu[] = {
    [APDU_selectByID]   = "\x00\xa4\x00\x00\x00",       // id = Master File?
    [APDU_selectFile]   = "\x00\xa4\x04\x00\x0e" "1PAY.SYS.DDF01",
    [APDU_readRecord]   = "\x00\xb2\x01\x14\x00",
    [APDU_readPSE]      = "\x00\xb2\x01\x02\x00\x00",           // idx = 02
    [APDU_getBalance]   = "\x80\x5c\x00\x02\x04\x00",
    [APDU_readBinary]   = "\x00\xb0\x95\x00\x00",               // sfi = 95
    "\xff\x00\x48\x00\x00",
    "\xff\x00\x00\x00\x02\xd4\x02",
    "\x00\xc0\x00\x00\x20",     // get response, le=0x20
};
static const uint8_t apdu_size[] = {
    [APDU_selectByID] = 5,
    [APDU_selectFile] = 19,
    [APDU_readRecord] = 5,
    [APDU_readPSE] = 6,
    [APDU_getBalance] = 6,
    [APDU_readBinary] = 5,
    5,
    7,
    5,
};
static const char *apdu_name[] = {
    [APDU_selectByID] = "selectByID",
    [APDU_selectFile] = "selectFile",
    [APDU_readRecord] = "readRecord",
    [APDU_readPSE] = "readPSE",
    [APDU_getBalance] = "getBalance",
    [APDU_readBinary] = "readBinary",
    "unk",
    "unk",
    "getResponse",
};


static serial_reserror(uint8_t * err) {
    switch(err[0]) {
        case 0x67:
            Serial.print(F("Length Incorrect"));
            return;
        case 0x69:
            Serial.print(F("Not Allowed"));
            // err[1] == 81, "imcompatible with file structure"
            return;
        case 0x6a:
            Serial.print(F("Wrong Param: "));
            switch(err[1]) {
                case 0x82:
                    Serial.print(F("File not found"));
                    return;
            }
            return;
        case 0x6d:
            Serial.print(F("ISN not supported"));
            return;
    }
}

static uint8_t apdu_send(Adafruit_PN532 nfc, char *name, uint8_t *cmd, uint8_t cmdlen, uint8_t *res, uint8_t reslen) {
    Serial.println(name);
    Serial.print(F("APDU Tx: "));
    hexdump(cmd,cmdlen);
    Serial.println();

    // FIXME: set private nfc._inListedTag == tg;
    if (!nfc.inDataExchange(cmd,cmdlen,res,&reslen)) {
        return 0;
    }

    Serial.print(F("APDU Rx: "));
    hexdump(res,reslen);
    if (reslen == 2) {
        Serial.print(' ');
        serial_reserror(res);
    }
    Serial.println();
    return reslen;
}

static void apdu_simple(Adafruit_PN532 nfc, uint8_t nr) {
    uint8_t *cmd = apdu[nr];
    uint8_t cmdlen = apdu_size[nr];

    uint8_t buf[32];
    apdu_send(nfc, apdu_name[nr], apdu[nr], apdu_size[nr], buf, sizeof(buf));
}

static void selectByID(Adafruit_PN532 nfc) {
    apdu_simple(nfc, APDU_selectByID);
}

static void getBalance(Adafruit_PN532 nfc) {
    apdu_simple(nfc, APDU_getBalance);
}

static void readPSE(Adafruit_PN532 nfc, uint8_t index) {
    apdu[APDU_readPSE][3] = index;
    apdu_simple(nfc, APDU_readPSE);
}

static void readBinary(Adafruit_PN532 nfc, uint8_t sfi) {
    apdu[APDU_readBinary][2] = sfi;
    apdu_simple(nfc, APDU_readBinary);
}

void decode_iso7816(Adafruit_PN532 nfc) {

    selectByID(nfc);
    apdu_simple(nfc, APDU_selectFile);
    readBinary(nfc,0x80 | 21);

    readPSE(nfc,2);
    getBalance(nfc);
}

