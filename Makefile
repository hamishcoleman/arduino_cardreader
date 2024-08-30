#
#
#

SKETCH := $(notdir $(PWD)).ino
CORE := arduino:avr@1.8.6
FQBN ?= arduino:avr:pro
PORT ?= /dev/ttyUSB0

DEPS += byteops.h byteops.cpp
DEPS += card_iso14443.h card_iso14443.cpp
DEPS += ledtimer.h ledtimer.cpp
DEPS += packets.h packets.cpp

# Ensure we start with a known config
ARDUINO_CONFIG_FILE ?= arduino-cli.yaml
export ARDUINO_CONFIG_FILE

# Since we are building with the submodule, we need to specify where it is
ARDUINO_DIRECTORIES_USER ?= .
export ARDUINO_DIRECTORIES_USER

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
# - download to a known dir?
# - add core to REALCLEAN?
.PHONY: hack_install_core
hack_install_core:
	bin/arduino-cli core install $(CORE)

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
	git submodule update --init

.PHONY: upload
upload: $(SKETCH)
	bin/arduino-cli compile --fqbn $(FQBN) --port $(PORT) --upload

# TODO: deps on the lib?
$(SKETCH).elf: $(SKETCH) $(DEPS)
	bin/arduino-cli compile --fqbn $(FQBN) --output-dir .

.PHONY: clean
clean:
	rm -f $(CLEAN_FILES)

.PHONY: realclean
realclean: clean
	rm -f $(REALCLEAN_FILES)
