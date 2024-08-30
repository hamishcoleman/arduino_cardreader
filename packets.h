/*
 * Copyright 2024 Hamish Coleman
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Packetised input and output handling
 */

#define packet_start()  Serial.print('\x02')
#define packet_end()    Serial.println('\x04')

