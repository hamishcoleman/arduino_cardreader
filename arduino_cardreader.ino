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

#include "byteops.h"        // for hexdump()
#include "card_iso14443.h"
#include "ledtimer.h"
#include "packets.h"

#define PN532_SS   (10)

// Note that the PN532 SCK, MOSI, and MISO pins need to be connected to the
// Arduino's // hardware SPI SCK, MOSI, and MISO pins.  On an Arduino Uno these
// are // SCK = 13, MOSI = 11, MISO = 12.  The SS line can be any digital IO
// pin.
Adafruit_PN532 nfc(PN532_SS);

// This list of possible types for InAutoPoll results was taken from the table
// in 7.3.13 of the Pn532 User Manual
#define TYPE_MIFARE     0x10
#define TYPE_FELICA_212 0x11
#define TYPE_FELICA_424 0x12
#define TYPE_ISO14443A  0x20

#define OUTPUT_RAWALL   1   // Always generate raw= messages
#define OUTPUT_RAWTAG   2   // Always generate rawtag= messages
uint8_t output_flags = 0;

#define LED1 7  // Intended to show status + activity (maybe green?)
#define LED2 8  // Reserved for showing an error (maybe red?)

struct led_status led1 = {
    .pin = LED1,
    .mode = LED_MODE_OFF,
};
struct led_status led2 = {
    .pin = LED2,
    .mode = LED_MODE_ON,
};

void setup(void) {
#ifndef ESP8266
    while (!Serial); // for Leonardo/Micro/Zero
#endif
    Serial.begin(115200);
    packet_start();
    Serial.print("sketch=" __FILE__);
    packet_end();

    pinMode(led1.pin, OUTPUT);
    pinMode(led2.pin, OUTPUT);

    // Set all status lights on to show we are booting
    digitalWrite(led1.pin, HIGH);
    digitalWrite(led2.pin, HIGH);

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
    digitalWrite(led1.pin, LOW);

    ledtimer_init();

    // Show the mainloop is ticking by turning off led2 shortly
    led2.next_state_millis = millis() + 500;

  Serial.println("Waiting for a Card ...");
}

uint8_t lastfound = 0;

void loop(void) {
    uint8_t polldata[64];   // Buffer to store the poll results
    uint8_t found = nfc.inAutoPoll(polldata, sizeof(polldata));

    if (!found) {
        if (lastfound) {
            // Show that the card reader is clear of detected cards
            packet_start();
            Serial.print("tag=NONE");
            packet_end();
            Serial.println();
            lastfound = 0;
        }
        return;
    }
    lastfound = found;

    // we found at least one card, blink the status light for a bit
    led1.mode = LED_MODE_BLINK1;
    led1.next_state_millis = millis() + 500;
    led2.mode = LED_MODE_ON;
    led2.next_state_millis = millis() + 3000;

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
        pos += len;
        found--;

        uint8_t tg = data[0];

        bool nfcid_decoded = false;
        uint8_t nfcidlength = 0;
        uint8_t *nfcid;

        switch (type) {
            case 0:
            case 1:
            case 2:
                // generic types are not possible
                // At the least, type 1 has been seen when an interrupted or
                // partial read is done (This was able to be replicated by
                // removing the card from the reader quickly)
                // - for two cards with 7 byte UIDs, when this occured the
                //   raw buffer was 0103442007
                // - for two cards with 4 byte UIDs, when this occurec the
                //   raw buffer was 0100

                break;

            case TYPE_MIFARE:
            case TYPE_ISO14443A:
                // uint16_t sens_res = data[1,2];
                // uint8_t sel_res = data[3];
                nfcidlength = data[4];
                nfcid = &data[5];
                nfcid_decoded = true;
                break;

            case TYPE_FELICA_212:
            case TYPE_FELICA_424:
                // uint8_t tg = data[0]
                // uint8_t pol_res = data[1]
                nfcidlength = 8;
                nfcid = &data[2];
                nfcid_decoded = true;
                break;

/*
            case 0x23: // Passive 106 kbps ISO/IEC14443-4B,
            case 0x40: // DEP passive 106 kbps,
            case 0x41: // DEP passive 212 kbps,
            case 0x42: // DEP passive 424 kbps,
            case 0x80: // DEP active 106 kbps,
            case 0x81: // DEP active 212 kbps,
            case 0x82: // DEP active 424 kbps.
                break;
*/
        }

        if (nfcid_decoded) {
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

        if (type == TYPE_ISO14443A && nfcidlength != 4) {
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

            do_iso14443a(tg);
        }
    }

    while (Serial.available()) {
        handle_serial();
    }
}
