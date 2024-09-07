/*
 * Copyright 2024 Hamish Coleman
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * An abstract representation of a generic card
 */
#pragma once

#define UID_TYPE_NONE       0   // No card is represented
#define UID_TYPE_UNKNOWN    1   // A Card with a type we dont understand
#define UID_TYPE_MIFARE     2
#define UID_TYPE_ISO14443A  3
#define UID_TYPE_FELICA     4

#define INFO_TYPE_NONE          0
#define INFO_TYPE_SERIAL        0x10
#define INFO_TYPE_SERIAL_HSL    0x10

#include <Print.h>

class Card {
    public:
        uint8_t uid_type;   // What card hardware type is represented
        uint8_t uid_len;    // How many bytes from uid are valid
        uint8_t uid[8];
        uint8_t info_type;
        char info[21];

        Card(void) {
            uid_type=UID_TYPE_NONE;
            info_type=INFO_TYPE_NONE;
            // ensure that the info buf is zero-terminated
            info[0] = 0;
        };

        bool operator == (const Card &a) {
            if (uid_type != a.uid_type || uid_len != a.uid_len) {
                return false;
            }
            if (memcmp(uid, a.uid, a.uid_len) != 0) {
                return false;
            }
            return true;
        }

        void set_uid(uint8_t *buf, uint8_t len);
        void set_uid_type(uint8_t type) { uid_type=type; };
        void print_uid(Print& p);

        void print_info(Print& p);

        // Called before overwriting info[] to print any previous info
        void print_info_msg(Print& p);

        // Chooses the best string to represent this card and prints that
        void print_cardid(Print& p);
};

