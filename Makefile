#
#
#

SKETCH := $(notdir $(PWD)).ino
BOARD := arduino:avr:pro
PORT ?= /dev/ttyUSB0

ARDUINO_CONFIG_FILE := arduino-cli.yaml
export ARDUINO_CONFIG_FILE

BOARD_FILENAME_SLUG := $(subst :,.,$(BOARD))
CLEAN_FILES += arduino_cardreader.ino.eep
CLEAN_FILES += arduino_cardreader.ino.elf
CLEAN_FILES += arduino_cardreader.ino.hex
CLEAN_FILES += arduino_cardreader.ino.with_bootloader.bin
CLEAN_FILES += arduino_cardreader.ino.with_bootloader.hex


all: $(SKETCH).elf

.PHONY: build-deps
build-deps:
	sudo apt-get -y install \
		curl \

# I dont want to use curl|sh, but until there is a debian pacakge, this is
# their documented install method...

ARDUINO_URL := https://raw.githubusercontent.com/arduino/arduino-cli/master/install.sh
bin/arduino-cli: /usr/bin/curl
	@echo 'NOTE: Ignore the warning about PATH'
	mkdir -p bin
	curl -fsSL $(ARDUINO_URL) | BINDIR=bin sh
REALCLEAN_FILES += bin/arduino-cli

# TODO:
# - install required library:
#   - automatically
#   - in a sane directory
#   - from a github url
#   - without overwriting or deleting existing stuff
#   - allowing parallel library development
#   - ideally, use a submodule to link the versions together
#   - "in the preferred version for editing"
# Theoretically the arduino-cli can manage libraries.  At most, it only
# provides one of the above dotpoints

.PHONY: hack_install_lib
hack_install_lib:
	mkdir -p ../libraries
	git clone https://github.com/hamishcoleman/arduino_pn532 ../libraries/arduino_pn532

.PHONY: upload
upload: $(SKETCH)
	bin/arduino-cli compile --fqbn $(BOARD) --port $(PORT) --upload

$(SKETCH).elf: $(SKETCH)
	bin/arduino-cli compile --fqbn $(BOARD) --output-dir .

.PHONY: clean
clean:
	rm -f $(CLEAN_FILES)

.PHONY: realclean
realclean: clean
	rm -f $(REALCLEAN_FILES)
