/*

    Read RFID cards and output machine readable information to the serial port


    Originally based on some example code from Adafruit_PN532:
        file     readMifare.pde
        author   Adafruit Industries
        license  BSD (see license.txt)

*/

/**************************************************************************/
#include <SPI.h>
#include <Adafruit_PN532.h>

#include "arduino_cardreader.h"
#include "byteops.h"        // for hexdump()
#include "card.h"
#include "card_iso14443.h"
#include "card_iso7816.h"
#include "card_mifare.h"
#include "hexdump.h"
#include "ledtimer.h"
#include "packets.h"

#define PN532_SS   (10)

// Note that the PN532 SCK, MOSI, and MISO pins need to be connected to the
// Arduino's // hardware SPI SCK, MOSI, and MISO pins.  On an Arduino Uno these
// are // SCK = 13, MOSI = 11, MISO = 12.  The SS line can be any digital IO
// pin.
Adafruit_PN532 nfc(PN532_SS);

uint8_t output_flags = 0;

void setup(void) {
#ifndef ESP8266
    while (!Serial); // for Leonardo/Micro/Zero
#endif
    Serial.begin(115200);
    packet_start(Serial);
    Serial.print("sketch=" __FILE__);
    packet_end(Serial);

    led[0].pin = LED1;
    led[1].pin = LED2;

    pinMode(led[0].pin, OUTPUT);
    pinMode(led[1].pin, OUTPUT);

    // Set all status lights on to show we are booting
    digitalWrite(led[0].pin, HIGH);
    digitalWrite(led[1].pin, HIGH);

  nfc.begin();

  uint32_t versiondata = nfc.getFirmwareVersion();
  if (! versiondata) {
    Serial.print("ERROR:no PN53x board found");
    while (1); // halt
  }
  // Got ok data, print it out!
  Serial.print("Found chip PN5"); Serial.println((versiondata>>24) & 0xFF, HEX);
  Serial.print("Firmware ver. "); Serial.print((versiondata>>16) & 0xFF, DEC);
  Serial.print('.'); Serial.println((versiondata>>8) & 0xFF, DEC);

  // configure board to read RFID tags
  nfc.SAMConfig();

    // Signal PN532 initialized by turning off led1
    digitalWrite(led[0].pin, LOW);

    // Show the timer and mainloop is ticking by turning off led2 shortly
    led[1].mode = LED_MODE_ON;
    led[1].next_state_millis = millis() + 500;

    ledtimer_init();

  Serial.println("Waiting for a Card ...");
}

Card last_card;

void loop(void) {
    while (Serial.available()) {
        handle_serial(Serial.read());
    }

    uint8_t polldata[64];   // Buffer to store the poll results
    uint8_t found = nfc.inAutoPoll(polldata, sizeof(polldata));

    if (!found) {
        if (last_card.uid_type != UID_TYPE_NONE) {
            // Show that the card reader is clear of detected cards
            last_card.uid_type = UID_TYPE_NONE;
            packet_start(Serial);
            Serial.print(F("uid="));
            last_card.print_uid(Serial);
            packet_end(Serial);
            Serial.println();
        }
        return;
    }

    // we found at least one card, blink the status light for a bit
    led[0].mode = LED_MODE_BLINK1;
    led[0].next_state_millis = millis() + 500;
    led[1].mode = LED_MODE_ON;
    led[1].next_state_millis = millis() + 3000;

    if (output_flags & OUTPUT_RAWALL) {
        // only output message if debugging output is on
        packet_start(Serial);
        Serial.print("rawpoll=");
        hexdump(Serial, polldata, sizeof(polldata));
        packet_end(Serial);
    }

    uint8_t pos = 0;
    while(found) {
        uint8_t type = polldata[pos++];
        uint8_t len = polldata[pos++];
        uint8_t *data = &polldata[pos];
        uint8_t *ats = NULL;
        pos += len;
        found--;

        // TODO: refactor - tg is part of the PN532 header, not the targetdata
        uint8_t tg = data[0];

        Card card;

        switch(type) {
            case TYPE_MIFARE:
            case TYPE_ISO14443A:
                // uint16_t sens_res = data[1,2];   ATQA
                // uint8_t sel_res = data[3];       SAK
                card.set_uid(&data[5], data[4]);
                if (len > (4 + card.uid_len)) {
                    ats = &data[5 + card.uid_len];
                }

                if (type == TYPE_MIFARE) {
                    card.set_uid_type(UID_TYPE_MIFARE);
                } else {
                    card.set_uid_type(UID_TYPE_ISO14443A);
                }

                break;
            case TYPE_FELICA_212:
            case TYPE_FELICA_424:
                // uint8_t pol_res = data[1] == len(targetdata)
                // uint8_t response = data[2] == 0x01 (polling RC)
                // part manufacturing data = data[11..19]
                card.set_uid(&data[3], 8);
                card.set_uid_type(UID_TYPE_FELICA);
                break;
            default:
                // TODO: highlight this better?
                // Any Unknown card is an opportunity to extend this list
                card.set_uid_type(UID_TYPE_UNKNOWN);
                card.uid_len = 0;
                break;
        }

/*
        Other possible card types:

        case 0x23: // Passive 106 kbps ISO/IEC14443-4B,
        case 0x40: // DEP passive 106 kbps,
        case 0x41: // DEP passive 212 kbps,
        case 0x42: // DEP passive 424 kbps,
        case 0x80: // DEP active 106 kbps,
        case 0x81: // DEP active 212 kbps,
        case 0x82: // DEP active 424 kbps.
*/

        if (card == last_card) {
            // Skip repeatly processing the same card
            return;
        }
        last_card = card;

        if (card.uid_type > UID_TYPE_UNKNOWN) {

            packet_start(Serial);
            Serial.print("uid=");
            card.print_uid(Serial);
            packet_end(Serial);
        }

        // Always do a raw dump if we didnt understand the data
        if ((card.uid_type <= UID_TYPE_UNKNOWN) || (output_flags & OUTPUT_RAWTAG)) {
            packet_start(Serial);
            Serial.print("rawtag=");
            hexdump(Serial, &type, 1);
            hexdump(Serial, &len, 1);
            hexdump(Serial, data, len);
            packet_end(Serial);
        }

        if (type == TYPE_MIFARE) {
            decode_mifare(nfc, card);
        }

        if (type == TYPE_ISO14443A) {
            if (ats) {
                // A combination of the ATS length and first two bytes
                uint32_t magic = buf_be2h24(ats);

                if (magic == 0x107880) {
                    // I have three cards that respond with "6700" for DESFire
                    // commands, which seems matches a 7816 "length error" code
                    // and they all have this magic
                    // Once again, I've not seen anything that claims to be a
                    // document for how to identify them (if only the ISO docs
                    // were actually free to download)
                    decode_iso7816(nfc);
                }
            }
            if (card.uid_len != 4) {
                // I have found nothing clearly documenting this, but some cards
                // using the ISO14443A discovery protocol dont actually respond
                // to any of the standard card function requests
                //
                // error datapoints:
                // ATQA=0008, len=4, apps returns 6700
                // ATQA=0044, len=7, apps returns 6700
                //
                // all working samples:
                // ATQA=0344, len=7, apps returns 00xxxxxx[yyyyyy]

                decode_iso14443a(nfc, tg, card);
            }
        }

        card.print_info_msg(Serial);
        card.print_cardid_msg(Serial);
    }
}
