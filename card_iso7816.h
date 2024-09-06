/*
 * Copyright 2024 Hamish Coleman
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * functions to interface with ISO7816 cards
 */
#pragma once

#include <Adafruit_PN532.h>
void decode_iso7816(Adafruit_PN532 nfc);
