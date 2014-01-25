/*
 * Nixie Clock
 *
 * Copyright (c) 2014 Chris Fallin <cfallin@c1f.net>
 * Placed under the modified BSD license
 *
 * main.c: main loop
 */

/* IO connections:
 *
 * PC0 (ADC0) in from 100:1 voltage divider on 170V supply
 * PB1 (OC1A) out to MOSFET on 170V supply (active high)
 * PC4 (SDA) out to nixie cathode shift register data
 * PC5 (SCL) out to nixie cathode shift register clock
 * PC3 out to nixie cathode shift register latch (active high)
 * PD0 in from pushbutton 1 (H)
 * PD1 in from pushbutton 2 (M)
 * PD2 in from pushbutton 3 (S-clear)
 */

#include <avr/io.h>
#include <avr/interrupt.h>

typedef struct {
    char H, M, S;
} hms_time_t;

hms_time_t current_time;

void init_time()
{
    current_time.H = current_time.M = current_time.S = 0;
}

int main()
{
    init_time(); // initialize the time
    sei(); // enable interrupts

    while(1); // the interrupts do the rest
}
