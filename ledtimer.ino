/*
 * Copyright 2024 Hamish Coleman
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * This is a timer driven, automatic LED state handler.
 * It is intended to be generic enough to be reusable in other projects
 */

void ledtimer_init() {
    // Configure timer1 to manage the led status, with a 100ms tick
    cli();
    TCCR1A = 0;
    TCCR1B = (1 << WGM12) | (1 << CS12);
    TCNT1 = 0;
    OCR1A = 6250;
    TIMSK1 = (1 << OCIE1A);
    TIFR1 |= (1 << OCF1A);
    sei();
}

static void led_update(struct led_status *led) {
    unsigned long now = millis();
    if ((led->next_state_millis - now) & 0x80000000) {
        // The next state is always "off"
        led->mode == LED_MODE_OFF;
        digitalWrite(led->pin, LOW);
        return;
    }

    switch (led->mode) {
        case LED_MODE_OFF:
            digitalWrite(led->pin, LOW);
            // off is off is off, so skip next state processing
            return;

        case LED_MODE_ON:
            digitalWrite(led->pin, HIGH);
            break;

        case LED_MODE_BLINK1:
            digitalWrite(led->pin, (now & 0x40)?HIGH:LOW);
            break;

        case LED_MODE_BLINK2:
            digitalWrite(led->pin, (now & 0x40)?LOW:HIGH);
            break;
    }
}

ISR(TIMER1_COMPA_vect) {
    // TODO: dont hardcode to two devices
    led_update(&led1);
    led_update(&led2);
}

