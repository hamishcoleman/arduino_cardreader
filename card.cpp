/*
 * Copyright 2024 Hamish Coleman
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * An abstract representation of a generic card
 */

#include <Print.h>
#include <stdarg.h>

#include "card.h"
#include "hexdump.h"
#include "packets.h"

void Card::set_uid(uint8_t *buf, uint8_t len) {
    uid_len = len;
    memcpy(uid, buf, len);
}

void Card::print_uid(Print& p) {
    switch(uid_type) {
        case UID_TYPE_NONE:
            p.print(F("NONE"));
            return;
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

void Card::set_info(const char *format, ...) {
    va_list ap;
    va_start(ap, format);
    vsnprintf(info, sizeof(info), format, ap);
    va_end(ap);
}

void Card::print_info(Print& p) {
    switch(info_type) {
        case INFO_TYPE_SERIAL_HSL:
            p.print(F("hsl"));
            break;
        case INFO_TYPE_SERIAL_TROIKA:
            p.print(F("troika"));
            break;
        case INFO_TYPE_SERIAL_MIKI:
            p.print(F("miki"));
            break;
        case INFO_TYPE_SERIAL_OPAL:
            p.print(F("opal"));
            break;
        case INFO_TYPE_SERIAL_CLIPPER:
            p.print(F("clipper"));
            break;
        default:
            p.print(F("ERROR"));
            return;
    }

    p.print('/');
    p.print(info);
}

void Card::print_info_msg(Print& p) {
    if (info_type == INFO_TYPE_NONE) {
        return;
    }

    packet_start(p);
    switch(info_type & 0xf0) {
        case INFO_TYPE_SERIAL:
            p.print(F("serial"));
            break;
        default:
            p.print(F("ERROR"));
            return;
    }
    p.print('=');
    print_info(p);
    packet_end(p);
}

void Card::print_cardid_msg(Print& p) {
    if (info_type == INFO_TYPE_NONE && uid_type <= UID_TYPE_UNKNOWN) {
        // nothing sane we can identify with
        return;
    }

    packet_start(p);
    p.print(F("cardid="));

    if (info_type != INFO_TYPE_NONE) {
        print_info(p);
    } else {
        // Our best card ID is the UID
        print_uid(p);
    }

    packet_end(p);
}
