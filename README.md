This simple sketch is intended to be used for a simple proximity card reader
system.  For instance, for access to a hack space.

## Software Setup:

### Arduino IDE
- You will need the correct PN532 library:
    https://github.com/hamishcoleman/arduino_pn532
  Make this library available to your arduino environment in its usual ways.
  (Note that if you have the upstream Adafruit_PN532 library, then there will
  be conflicts, so remove the other one first)
- This sketch should compile and verify cleanly

### Building from the CLI
In order to support traditional software development and make it easier to
build CI systems, this project also has a Makefile to build it.

Install the dev tools and required library:
```
make bin/arduino-cli hack_install_core hack_install_lib
```

Other build targets:
- `make all`
- `make clean`
- `make upload`

## Hardware Setup:
- Get a PN532 module (many suitable are available online)
- Wire up the Arduino Hardware SPI port to the PN532
- Optionally, connect LEDs to Arduino Pins 7 and 8

## Example output:
After programming, the serial console will show detected cards:

```
sketch=/home/runner/work/arduino_cardreader/arduino_cardreader/arduino_cardreader.ino
Found chip PN532
Firmware ver. 1.4
Waiting for a Card ...
uid=iso14443a/0435178A597532
serial=opal/3085220093141592
cardid=opal/3085220093141592
uid=NONE

uid=iso14443a/AF8E8E13
cardid=iso14443a/AF8E8E13
uid=mifare/E2E2F98B
cardid=mifare/E2E2F98B
uid=NONE

```

This example shows two reads.  Firstly, an opal card is held up and then
removed - showing a card that can have its serial number determined.
Secondly, both a Translink Go card and a Shenzhen metro card are
held up at the same time - showing simultaneous detection and reading.

## Communications Protocol

All communications (both input and output) are intended to be both useful for
humans to view and debug and simple for a machine to reliably parse the data.

All messages intended for machine parsing (both input and output) are framed
with a start and a stop character.  The message starts with a STX (0x02) char
and ends with a EOT (0x04) char.

Any text received before an STX char or after an EOT char should not be
interpreted.  This allows debugging output or informational text to be
easily intermixed with operational messages.

Currently, the message format is printable text only and thus there is no
need to escape any control characters.

Currently, every message sent from the card reader is comprised of a "key" and
a "value", separated by an "=" character.

This format is expected to evolve after more testing.

### Output Messages

| key | brief description |
| --- | ----------------- |
| cardid | in the cardreader's opinion, the best identifying string |
| rawpoll | An optional message for debugging the raw poll data |
| rawtag | An optional message for debugging tag data |
| serial | If possible, the serial number printed on the card is output |
| uid | The internal card unique ID |

### Message "serial="

An attempt is made to decode the serial number printed on the outside of the
card and presented via this message.

Most cards either do not have this information available without the private
keys, or do not store this information on the card itself.

### Message "uid="

This status output contains the "Anticollision Unique Identifier" of any card
presented to the reader.  This will generally not be related to any number
printed on the card itself and may even not be unique at all.

It is the simplest and lowest level for identifying between different cards.

The special value "NONE" indicates that there is no longer any card in front
of the reader.

In most cases, the uid is only generated once for each time the card is held
up to the reader and once removed a NONE tag will be output to show that the
card has been removed.

The card uid is prefixed with the name of the card type to keep the (sometimes
quite small) ID name space separate for each type of RFID hardware.

### Message "rawtag="

If the card type (see prefix mentioned for uid=) cannot be determined, the
rawtag= message is output.  This message can also be unconditionally enabled
for output.  This status output allows for possible further processing of the
card data, sending a hexdump of the raw data for this tag.

The unconditional enabling of this message defaults to disabled and needs to be
enabled with the "t" command.

The exact format of this message is determined by the contents of the buffer
that the PN532 hardware returns and the documentation for that should be
checked for full details.  The basic format is one byte for "type", one byte
for "len" and then len bytes of the card data.

Note that erroneous or partial card reads have been known to show up as type
0x01

### Message "rawpoll="

When enabled, this status output is used for detailed debugging, sending a
large chunk of the InAutoPoll response PDU.

This message defaults to disabled and needs to be enabled with the "r" command.

### Commands

A number of simple commands can be sent to manage the device.  When using a
serial terminal, the message framing can be added by starting with a `Ctrl-B`
and ending with a `Ctrl-D`

| command | Action |
| ------- | ------ |
| H | Sends a quick hello debug text back to the user |
| 0 | Turns off both LEDs |
| 1 | Turns LED1 on |
| 2 | Turns LED2 on |
| 3 | Blinks LED1 |
| 4 | Blinks LED2 |
| 5 | Blinks LED1 out of phase |
| 6 | Blinks LED2 out of phase |
| 7 | Blinks both LEDs, one in each phase |
| r | Enable rawpoll= messages |
| R | Disable rawpoll= messages |
| t | Enable rawtag= messages |
| T | Disable rawtag= messages |

Note: The led status will last for 20 seconds before being turned back off
again.  If the output is needed for longer, then the command needs to be
repeated.

## Example wiring:

<img src="wiring_example.jpg" width=800/>

## Expected usage

- LED1 is anticipated to be GREEN
- LED2 is anticipated to be RED

| LED1 | LED2 | Meaning |
| ---- | ---- | ------- |
| off  | off  | standby |
| blinking green | * | currently reading a card (turns off 500ms after card is gone) |
| * | red | waiting for server (turns off after 3000ms, or when server replies) |
| green | off | door is open (only when server is working) |
| blinking green | blinking red | card denied (only when server is working) |

Boot up status:
| LED1 | LED2 | Meaning |
| ---- | ---- | ------- |
| green | red | bootup start |
| off | red | RFID initialised |
| off | off | bootup finished (500ms after bootup complete) |

Client app
- Waits for a uid= message
- looks card up in database
- If card good, sends LEDoff and LED1on commands
- If card bad, sends LEDbothBlink commands
