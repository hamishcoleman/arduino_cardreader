/*
 * Copyright 2024 Hamish Coleman
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * functions to interface with ISO14443A cards
 *
 * FIXME:
 * This module currently relies on a hacked Adafruit_PN532 that managed the
 * private _inListedTag variable differently
 */
#pragma once

#include <Adafruit_PN532.h>
#include "card.h"

void decode_mifare(Adafruit_PN532& nfc, Card& card);
