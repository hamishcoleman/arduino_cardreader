/*
 * Copyright 2024 Hamish Coleman
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Packetised input and output handling
 */
#pragma once

#define packet_start()  Serial.print('\x02')
#define packet_end()    Serial.println('\x04')

void handle_serial(uint8_t ch);
