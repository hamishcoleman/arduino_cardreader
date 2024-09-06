/*
 * Copyright 2024 Hamish Coleman
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Shared definitiions
 */
#pragma once

#include <Adafruit_PN532.h>

#define LED1 7  // Intended to show status + activity (maybe green?)
#define LED2 8  // Reserved for showing an error (maybe red?)

// This list of possible types for InAutoPoll results was taken from the table
// in 7.3.13 of the Pn532 User Manual
#define TYPE_MIFARE     0x10
#define TYPE_FELICA_212 0x11
#define TYPE_FELICA_424 0x12
#define TYPE_ISO14443A  0x20

#define OUTPUT_RAWALL   1   // Always generate raw= messages
#define OUTPUT_RAWTAG   2   // Always generate rawtag= messages
#define OUTPUT_EXTRA    4   // Poll the card for extra data
extern uint8_t output_flags;
