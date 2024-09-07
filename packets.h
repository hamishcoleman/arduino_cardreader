/*
 * Copyright 2024 Hamish Coleman
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Packetised input and output handling
 */
#pragma once

#define packet_start(p)  p.print('\x02')
#define packet_end(p)    p.println('\x04')

void handle_serial(uint8_t ch);
