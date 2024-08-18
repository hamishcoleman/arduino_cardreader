This simple sketch is intended to be used for a simple proximity card reader
system.  For instance, for access to a hack space.

## Software Setup:
- You will need the correct PN532 library:
    https://github.com/hamishcoleman/arduino_pn532
  Make this library available to your arduino environment in its usual ways.
  (Note that if you have the upstream Adafruit_PN532 library, then there will
  be conflicts, so remove the other one first)
- This sketch should compile and verify cleanly

## Hardware Setup:
- Get a PN532 module (many suitable are available online)
- Wire up the Arduino Hardware SPI port to the PN532
- Optionally, connect LEDs to Arduino Pins 7 and 8

## Example output:
After programming, the serial console will show detected cards:

```
Hello!
Found chip PN532
Firmware ver. 1.6
Waiting for a Card ...
tag: 0 type: 20 uid: 4 35 17 8A 59 75 32 
tag: 0 type: 20 uid: AF 8E 8E 13
tag: 1 type: 10 uid: E2 E2 A2 3F
```

This example shows two reads.  Firstly, an oyster card is held up and then
removed.  Secondly, both a Translink Go card and a Shenzhen metro card are
held up at the same time - showing simultaneous detection and reading.

This format is expected to evolve after more testing.
