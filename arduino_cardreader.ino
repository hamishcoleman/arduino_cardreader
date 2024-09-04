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
#include "card_iso14443.h"
#include "card_iso7816.h"
#include "card_mifare.h"
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
    packet_start();
    Serial.print("sketch=" __FILE__);
    packet_end();

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

uint8_t last_uidlen = 0;
uint8_t last_uid[12];

void loop(void) {
    while (Serial.available()) {
        handle_serial(Serial.read());
    }

    uint8_t polldata[64];   // Buffer to store the poll results
    uint8_t found = nfc.inAutoPoll(polldata, sizeof(polldata));

    if (!found) {
        if (last_uidlen) {
            // Show that the card reader is clear of detected cards
            packet_start();
            Serial.print("tag=NONE");
            packet_end();
            Serial.println();
            last_uidlen = 0;
            memset(last_uid, 0, sizeof(last_uid));
        }
        return;
    }

    // We overload the uidlen here - for non decoded cards, the len will end
    // up being 1 or 2, which should still not match the deduplication logic
    // while at the same time working with the tag=NONE logic.
    if (!last_uidlen) {
        last_uidlen = found;
    }

    // we found at least one card, blink the status light for a bit
    led[0].mode = LED_MODE_BLINK1;
    led[0].next_state_millis = millis() + 500;
    led[1].mode = LED_MODE_ON;
    led[1].next_state_millis = millis() + 3000;

    if (output_flags & OUTPUT_RAWALL) {
        // only output message if debugging output is on
        packet_start();
        Serial.print("raw=");
        hexdump(polldata, sizeof(polldata));
        packet_end();
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

        bool nfcid_decoded = false;
        uint8_t nfcidlength = 0;
        uint8_t *nfcid;

        if (type == TYPE_MIFARE || type == TYPE_ISO14443A) {
            // uint16_t sens_res = data[1,2];   ATQA
            // uint8_t sel_res = data[3];       SAK
            nfcidlength = data[4];
            nfcid = &data[5];
            nfcid_decoded = true;

            if (len > (4 + nfcidlength)) {
                ats = &data[5 + nfcidlength];
            }
        }

        if (type == TYPE_FELICA_212 || type == TYPE_FELICA_424) {
            // uint8_t pol_res = data[1] == len(targetdata)
            // uint8_t response = data[2] == 0x01 (polling RC)
            // part manufacturing data = data[11..19]
            nfcidlength = 8;
            nfcid = &data[3];
            nfcid_decoded = true;
        }

/*
        case 0x23: // Passive 106 kbps ISO/IEC14443-4B,
        case 0x40: // DEP passive 106 kbps,
        case 0x41: // DEP passive 212 kbps,
        case 0x42: // DEP passive 424 kbps,
        case 0x80: // DEP active 106 kbps,
        case 0x81: // DEP active 212 kbps,
        case 0x82: // DEP active 424 kbps.
*/

        if (nfcid_decoded) {
            if ((last_uidlen==nfcidlength) && (memcmp(last_uid,nfcid,nfcidlength)==0)) {
                // Skip repeatly processing the same card
                return;
            }

            packet_start();
            Serial.print("tag=");
            switch(type) {
                case TYPE_MIFARE:
                    Serial.print("mifare/");
                    break;
                case TYPE_ISO14443A:
                    Serial.print("iso14443a/");
                    break;
                case TYPE_FELICA_212:
                case TYPE_FELICA_424:
                    Serial.print("felica/");
                    break;
            }
            hexdump(nfcid, nfcidlength);
            packet_end();
            last_uidlen=nfcidlength;
            memcpy(last_uid, nfcid, nfcidlength);
        }

        // Always do a raw dump if we didnt understand the data
        if ((!nfcid_decoded) || (output_flags & OUTPUT_RAWTAG)) {
            packet_start();
            Serial.print("rawtag=");
            hexdump(&type, 1);
            hexdump(&len, 1);
            hexdump(data, len);
            packet_end();
        }

        if (type == TYPE_MIFARE) {
            decode_mifare(nfc, nfcid, nfcidlength);
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
            if (nfcidlength != 4) {
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

                decode_iso14443a(nfc, tg);
            }
        }

    }
}
