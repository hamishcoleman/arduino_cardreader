/*
 * Copyright 2024 Hamish Coleman
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * An abstract representation of a generic card
 */

#include <Print.h>
#include "card.h"
#include "hexdump.h"

void Card::set_uid(uint8_t *buf, uint8_t len) {
    uid_len = len;
    memcpy(uid, buf, len);
}

void Card::print_uid(Print& p) {
    switch(uid_type) {
        case UID_TYPE_MIFARE:
            p.print(F("mifare"));
            break;
        case UID_TYPE_ISO14443A:
            p.print(F("iso14443a"));
            break;
        case UID_TYPE_FELICA:
            p.print(F("felica"));
            break;
        default:
            p.print(F("ERROR"));
            return;
    }

    p.print('/');
    hexdump(p, uid, uid_len);
}
