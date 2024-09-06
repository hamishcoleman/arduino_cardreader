/*
 * Copyright 2024 Hamish Coleman
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * A small collection of useful tools for handling bytes and buffers.
 * It is intended to be generic enough to be reusable in other projects
 */
#pragma once

/* Convert from a memory buffer to an integer */
uint32_t buf_be2hl(uint8_t *buf);
uint32_t buf_be2h24(uint8_t *buf);
uint32_t buf_le2hl(uint8_t *buf);

/* Print an int with a zeropadded prefix */
void serial_intzeropad(uint32_t i, uint8_t zeropad);
