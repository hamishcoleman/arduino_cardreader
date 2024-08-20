/**hamish************************************************************************/
/*!
    @file     readMifare.pde
    @author   Adafruit Industries
    @license  BSD (see license.txt)

    This example will wait for any ISO14443A card or tag, and
    depending on the size of the UID will attempt to read from it.

    If the card has a 4-byte UID it is probably a Mifare
    Classic card, and the following steps are taken:

    - Authenticate block 4 (the first block of Sector 1) using
      the default KEYA of 0XFF 0XFF 0XFF 0XFF 0XFF 0XFF
    - If authentication succeeds, we can then read any of the
      4 blocks in that sector (though only block 4 is read here)

    If the card has a 7-byte UID it is probably a Mifare
    Ultralight card, and the 4 byte pages can be read directly.
    Page 4 is read by default since this is the first 'general-
    purpose' page on the tags.


This is an example sketch for the Adafruit PN532 NFC/RFID breakout boards
This library works with the Adafruit NFC breakout
  ----> https://www.adafruit.com/products/364

Check out the links above for our tutorials and wiring diagrams
These chips use SPI or I2C to communicate.

Adafruit invests time and resources providing this open source code,
please support Adafruit and open-source hardware by purchasing
products from Adafruit!

*/
/**************************************************************************/
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_PN532.h>

// If using the breakout with SPI, define the pins for SPI communication.
//#define PN532_SCK  (2)
//#define PN532_MOSI (3)
#define PN532_SS   (10)
//#define PN532_MISO (5)

// If using the breakout or shield with I2C, define just the pins connected
// to the IRQ and reset lines.  Use the values below (2, 3) for the shield!
//#define PN532_IRQ   (2)
//#define PN532_RESET (3)  // Not connected by default on the NFC Shield

// Uncomment just _one_ line below depending on how your breakout or shield
// is connected to the Arduino:

// Use this line for a breakout with a software SPI connection (recommended):
//Adafruit_PN532 nfc(PN532_SCK, PN532_MISO, PN532_MOSI, PN532_SS);

// Use this line for a breakout with a hardware SPI connection.  Note that
// the PN532 SCK, MOSI, and MISO pins need to be connected to the Arduino's
// hardware SPI SCK, MOSI, and MISO pins.  On an Arduino Uno these are
// SCK = 13, MOSI = 11, MISO = 12.  The SS line can be any digital IO pin.
Adafruit_PN532 nfc(PN532_SS);

// Or use this line for a breakout or shield with an I2C connection:
//Adafruit_PN532 nfc(PN532_IRQ, PN532_RESET);

#if defined(ARDUINO_ARCH_SAMD)
// for Zero, output on USB Serial console, remove line below if using programming port to program the Zero!
// also change #define in Adafruit_PN532.cpp library file
   #define Serial SerialUSB
#endif

#define LED1 7  // Intended to show status + activity (maybe green?)
#define LED2 8  // Reserved for showing an error (maybe red?)

#define packet_start()  Serial.print('\x02')
#define packet_end()    Serial.println('\x04')

// If only Serial.print(xyzzy, HEX) did leading zeros
// (The lack of leading zeros has been raised before and they do not appear
// to be interested in addressing it)
void hexdump(uint8_t *buf, uint8_t size) {
    char digits[] = "0123456789ABCDEF";
    while(size) {
        char ch = *buf;
        char hi = (ch >> 4) & 0x0f;
        char lo = ch & 0x0f;
        Serial.print(digits[hi]);
        Serial.print(digits[lo]);

        buf++;
        size--;
    }
}

void setup(void) {
#ifndef ESP8266
    while (!Serial); // for Leonardo/Micro/Zero
#endif
    Serial.begin(115200);
    packet_start();
    Serial.print("sketch=" __FILE__);
    packet_end();

  pinMode(LED1, OUTPUT);
  pinMode(LED2, OUTPUT);

  nfc.begin();

  uint32_t versiondata = nfc.getFirmwareVersion();
  if (! versiondata) {
    Serial.print("Didn't find PN53x board");
    while (1); // halt
  }
  // Got ok data, print it out!
  Serial.print("Found chip PN5"); Serial.println((versiondata>>24) & 0xFF, HEX);
  Serial.print("Firmware ver. "); Serial.print((versiondata>>16) & 0xFF, DEC);
  Serial.print('.'); Serial.println((versiondata>>8) & 0xFF, DEC);

  // configure board to read RFID tags
  nfc.SAMConfig();

  Serial.println("Waiting for a Card ...");
}

unsigned long led1_on = 0;
uint8_t lastfound = 0;

void loop(void) {

  uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0, 0 };  // Buffer to store the returned UID
  uint8_t uidLength;                        // Length of the UID (4 or 7 bytes depending on ISO14443A card type)

    uint8_t polldata[64];   // Buffer to store the poll results

  // Wait for an ISO14443A type cards (Mifare, etc.).  When one is found
  // 'uid' will be populated with the UID, and uidLength will indicate
  // if the uid is 4 bytes (Mifare Classic) or 7 bytes (Mifare Ultralight)

#if 0
  success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength);
  Serial.println("Found a MIFARE card");
#endif

#if 0
  success = nfc.readPassiveTargetID(PN532_FELICA_212, uid, &uidLength);
  Serial.println("Found a FELICA card");
#endif

    uint8_t found = nfc.inAutoPoll(polldata, sizeof(polldata));

    if (found) {
        // TODO: if reportmode includes rawall
        packet_start();
        Serial.print("raw=");
        hexdump(polldata, sizeof(polldata));
        packet_end();
    }

    if (!found && lastfound) {
        // Show that the card reader is clear of detected cards
        packet_start();
        Serial.print("tag=NONE");
        packet_end();
        Serial.println();
    }
    lastfound = found;

    // If we found any cards, turn the status light on for a bit
    if (found) {
        if (!led1_on) {
            led1_on = millis();
            digitalWrite(LED1, HIGH);
        }
    }

    if (led1_on && (millis() - led1_on > 1000UL)) {
        led1_on = 0;
        digitalWrite(LED1, LOW);
    }

    uint8_t pos = 0;
    while(found) {
        uint8_t type = polldata[pos++];
        uint8_t len = polldata[pos++];
        uint8_t *data = &polldata[pos];
        pos += len;
        found--;

        bool nfcid_decoded = false;
        uint8_t nfcidlength = 0;
        uint8_t *nfcid;

        switch (type) {
            case 0:
            case 1:
            case 2:
                // generic types are not possible
                break;

            case 0x10: // Mifare card,
            case 0x20: // Passive 106 kbps ISO/IEC14443-4A,
                // uint8_t tg = data[0]
                // uint16_t sens_res = data[1,2];
                // uint8_t sel_res = data[3];
                nfcidlength = data[4];
                nfcid = &data[5];
                nfcid_decoded = true;
                break;

            case 0x11: // FeliCa 212 kbps card,
            case 0x12: // FeliCa 424 kbps card,
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

        // TODO: the detected card type may not be best to include in ID
        if (nfcid_decoded) {
            packet_start();
            Serial.print("tag=");
            hexdump(&type, 1);
            Serial.print("/");
            hexdump(nfcid, nfcidlength);
            packet_end();
        }

        // TODO: if reportmode includes rawtag or not nfcid_decoded
        packet_start();
        Serial.print("rawtag=");
        hexdump(&type, 1);
        hexdump(&len, 1);
        hexdump(data, len);
        packet_end();

    }

#if 0
  if (success) {
    // Display some basic information about the card
    Serial.println("Found a card");
    Serial.print("  UID Length: ");Serial.print(uidLength, DEC);Serial.println(" bytes");
    Serial.print("  UID Value: ");
    nfc.PrintHex(uid, uidLength);
    Serial.println("");

    if (uidLength == 4)
    {
      // We probably have a Mifare Classic card ...
      Serial.println("Seems to be a Mifare Classic card (4 byte UID)");

      // Now we need to try to authenticate it for read/write access
      // Try with the factory default KeyA: 0xFF 0xFF 0xFF 0xFF 0xFF 0xFF
      Serial.println("Trying to authenticate block 4 with default KEYA value");
      uint8_t keya[6] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };

      // Start with block 4 (the first block of sector 1) since sector 0
      // contains the manufacturer data and it's probably better just
      // to leave it alone unless you know what you're doing
      success = nfc.mifareclassic_AuthenticateBlock(uid, uidLength, 4, 0, keya);

      if (success)
      {
        Serial.println("Sector 1 (Blocks 4..7) has been authenticated");
        uint8_t data[16];

        // If you want to write something to block 4 to test with, uncomment
        // the following line and this text should be read back in a minute
        //memcpy(data, (const uint8_t[]){ 'a', 'd', 'a', 'f', 'r', 'u', 'i', 't', '.', 'c', 'o', 'm', 0, 0, 0, 0 }, sizeof data);
        // success = nfc.mifareclassic_WriteDataBlock (4, data);

        // Try to read the contents of block 4
        success = nfc.mifareclassic_ReadDataBlock(4, data);

        if (success)
        {
          // Data seems to have been read ... spit it out
          Serial.println("Reading Block 4:");
          nfc.PrintHexChar(data, 16);
          Serial.println("");

          // Wait a bit before reading the card again
          delay(1000);
        }
        else
        {
          Serial.println("Ooops ... unable to read the requested block.  Try another key?");
        }
      }
      else
      {
        Serial.println("Ooops ... authentication failed: Try another key?");
      }
    }

    if (uidLength == 7)
    {
      // We probably have a Mifare Ultralight card ...
      Serial.println("Seems to be a Mifare Ultralight tag (7 byte UID)");

      // Try to read the first general-purpose user page (#4)
      Serial.println("Reading page 4");
      uint8_t data[32];
      success = nfc.mifareultralight_ReadPage (4, data);
      if (success)
      {
        // Data seems to have been read ... spit it out
        nfc.PrintHexChar(data, 4);
        Serial.println("");

        // Wait a bit before reading the card again
        delay(1000);
      }
      else
      {
        Serial.println("Ooops ... unable to read the requested page!?");
      }
    }
  }
#endif
}
