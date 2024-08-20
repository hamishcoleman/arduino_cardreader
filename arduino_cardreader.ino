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

#define PN532_SS   (10)

// Note that the PN532 SCK, MOSI, and MISO pins need to be connected to the
// Arduino's // hardware SPI SCK, MOSI, and MISO pins.  On an Arduino Uno these
// are // SCK = 13, MOSI = 11, MISO = 12.  The SS line can be any digital IO
// pin.
Adafruit_PN532 nfc(PN532_SS);

#define OUTPUT_RAWALL   1   // Always generate raw= messages
#define OUTPUT_RAWTAG   2   // Always generate rawtag= messages
uint8_t output_raw = 0;

#define LED1 7  // Intended to show status + activity (maybe green?)
#define LED2 8  // Reserved for showing an error (maybe red?)

#define LED_MODE_OFF    0
#define LED_MODE_BLINK1 1   // phase1
#define LED_MODE_BLINK2 2   // phase2
#define LED_MODE_ON     9

struct led_status {
    uint8_t pin;
    uint8_t mode;
    unsigned long next_state_millis;
};

struct led_status led1 = {
    .pin = LED1,
    .mode = LED_MODE_OFF,
};
struct led_status led2 = {
    .pin = LED2,
    .mode = LED_MODE_ON,
};

void led_update(struct led_status *led) {
    unsigned long now = millis();
    if ((led->next_state_millis - now) & 0x80000000) {
        // The next state is always "off"
        led->mode == LED_MODE_OFF;
        digitalWrite(led->pin, LOW);
        return;
    }

    switch (led->mode) {
        case LED_MODE_OFF:
            digitalWrite(led->pin, LOW);
            // off is off is off, so skip next state processing
            return;

        case LED_MODE_ON:
            digitalWrite(led->pin, HIGH);
            break;

        case LED_MODE_BLINK1:
            digitalWrite(led->pin, (now & 0x40)?HIGH:LOW);
            break;

        case LED_MODE_BLINK2:
            digitalWrite(led->pin, (now & 0x40)?LOW:HIGH);
            break;
    }
}

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

void handle_serial_cmd(uint8_t *cmd, uint8_t len) {
    // Only trivial one char commands are implemented
    if (len != 1) {
        return;
    }

    switch (cmd[0]) {
        case 'H':
            Serial.println("Hello");
            return;
        case '0':
            led1.mode = LED_MODE_OFF;
            led2.mode = LED_MODE_OFF;
            return;
        case '1':
            led1.mode = LED_MODE_ON;
            led1.next_state_millis = millis() + 20000;
            return;
        case '2':
            led2.mode = LED_MODE_ON;
            led2.next_state_millis = millis() + 20000;
            return;
        case '3':
            led1.mode = LED_MODE_BLINK1;
            led1.next_state_millis = millis() + 20000;
            return;
        case '4':
            led2.mode = LED_MODE_BLINK1;
            led2.next_state_millis = millis() + 20000;
            return;
        case '5':
            led1.mode = LED_MODE_BLINK2;
            led1.next_state_millis = millis() + 20000;
            return;
        case '6':
            led2.mode = LED_MODE_BLINK2;
            led2.next_state_millis = millis() + 20000;
            return;
        case '7':
            led1.mode = LED_MODE_BLINK1;
            led1.next_state_millis = millis() + 20000;
            led2.mode = LED_MODE_BLINK2;
            led2.next_state_millis = millis() + 20000;
            return;
        case 'r':
            output_raw |= OUTPUT_RAWALL;
            return;
        case 'R':
            output_raw &= ~OUTPUT_RAWALL;
            return;
        case 't':
            output_raw |= OUTPUT_RAWTAG;
            return;
        case 'T':
            output_raw &= ~OUTPUT_RAWTAG;
            return;
    }
}

// Buffer to accumulate incoming message packets
uint8_t cmd[8];
uint8_t cmdpos = 0xff;

void handle_serial() {
    uint8_t ch = Serial.read();
    if (ch == '\x02') {
        // Start of frame
        cmdpos = 0;
        return;
    }
    if (ch == '\x04') {
        // End of frame
        handle_serial_cmd(cmd, cmdpos);
        cmdpos = 0xff;
        return;
    }
    if (cmdpos == 0xff) {
        // Discard bytes outside of packet frame
        return;
    }
    if (cmdpos >= sizeof(cmd)) {
        // Overflow, alert and discard until end of packet
        Serial.print('\x15');
        cmdpos = 0xff;
        return;
    }
    cmd[cmdpos++] = ch;
}

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

    // Configure timer1 to manage the led status, with a 100ms tick
    cli();
    TCCR1A = 0;
    TCCR1B = (1 << WGM12) | (1 << CS12);
    TCNT1 = 0;
    OCR1A = 6250;
    TIMSK1 = (1 << OCIE1A);
    TIFR1 |= (1 << OCF1A);
    sei();

    // Show the mainloop is ticking by turning off led2 shortly
    led2.next_state_millis = millis() + 500;

  Serial.println("Waiting for a Card ...");
}

ISR(TIMER1_COMPA_vect) {
    led_update(&led1);
    led_update(&led2);
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

    if (output_raw & OUTPUT_RAWALL) {
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

        // Always do a raw dump if we didnt understand the data
        if ((!nfcid_decoded) || (output_raw & OUTPUT_RAWTAG)) {
            packet_start();
            Serial.print("rawtag=");
            hexdump(&type, 1);
            hexdump(&len, 1);
            hexdump(data, len);
            packet_end();
        }
    }

    while (Serial.available()) {
        handle_serial();
    }
}
