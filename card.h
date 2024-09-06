/*
 * Copyright 2024 Hamish Coleman
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * An abstract representation of a generic card
 */

#define UID_TYPE_NONE       0
#define UID_TYPE_MIFARE     1
#define UID_TYPE_ISO14443A  2
#define UID_TYPE_FELICA     3

#include <Print.h>

class Card {
    public:
        uint8_t uid_type;   // What card hardware type is represented
        uint8_t uid_len;    // How many bytes from uid are valid
        uint8_t uid[8];

        Card(void) { uid_type=UID_TYPE_NONE; };

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
};

