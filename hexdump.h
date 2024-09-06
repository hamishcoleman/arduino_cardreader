/*
 * Copyright 2024 Hamish Coleman
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Just implment a simple and sane hexdump
 */

#include <Print.h>

/* Dump a memory buffer to serial */
void hexdump(Print& p, uint8_t *buf, uint8_t size);
