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
tag=20/0435178A597532
rawtag=201201034420070435178A597532067577810280
tag=NONE

tag=20/AF8E8E13
rawtag=20190100002004AF8E8E13107880A0028684665257453250524F4D
tag=10/E2E2F98B
rawtag=10090200040804E2E2A23F
tag=NONE

```

This example shows two reads.  Firstly, an oyster card is held up and then
removed.  Secondly, both a Translink Go card and a Shenzhen metro card are
held up at the same time - showing simultaneous detection and reading.

## Parsing the output

The output is intended to be both useful for humans to view and debug and
simple for a machine to reliably parse the data.

Any message intended for machine parsing is framed with a start and a stop
character.  The message starts with a STX (0x02) char and ends with a EOT
(0x04) char.

Any text received before an STX char or after an EOT char should not be
interpreted.  This allows debugging output or informational text to be
easily intermixed with operational messages.

Currently, the message format is printable text only and thus there is no
need to escape any control characters.

Each message is comprised of a "tag" and a "value", separated by an "="
character.

This format is expected to evolve after more testing.

### Message "tag="

The main output is the "tag=" message.  This shows the unique hex ID determined
from the cards presented to the reader.

The special tag "NONE" indicates that there is no longer any card in front
of the reader.

Currently, if a card is held to the reader for an extended time, this message
will repeat for as long as the card is held up - this may change to avoid
repeating identical values in the future.

The card tag is prefixed with the detected card type to keep the (sometimes
quite small) ID name space separate for each type of RFID hardware.  Currently,
this card type is an internal PN532 identifier, which probably needs to change.

### Message "rawtag="

To allow possible further processing of the card data, a hexdump of the raw
data for this tag is output.  In the future, this message will be optional and
only output when turned on.

### Message "raw="

For detailed debugging, a large chunk of the InAutoPoll response PDU is output.
In the future, this message will be optional and only output when turned on.

## Example wiring:

<img src="wiring_example.jpg" width=800/>
