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

bool iso14443a_select_app(Adafruit_PN532&, uint8_t tg, uint32_t app);
uint8_t iso14443a_read_file(Adafruit_PN532&, uint8_t tg, uint8_t file, uint8_t offset, uint8_t size, uint8_t *buf, uint8_t buflen);
uint8_t do_iso14443a_apps(Adafruit_PN532&, uint8_t tg, uint8_t *res, uint8_t reslen);
void decode_iso14443a(Adafruit_PN532&, uint8_t tg);
