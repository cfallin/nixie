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

/* local functions */
static void init_time();
static void one_second();

static void init_display();
static void write_digit(char digit);
static void write_display();
static void init_buttons();
static void check_buttons();
static void init_smps();
static void adjust_smps();

typedef struct {
    unsigned char H, M, S;
    unsigned char ticks, correction;
} hms_time_t;

hms_time_t current_time;

unsigned char debounce_H, debounce_M, debounce_S;
unsigned char registered_H, registered_M, registered_S;

void init_time()
{
    current_time.H = current_time.M = current_time.S = 0;
    current_time.ticks = current_time.correction = 0;
    
    // set up timer 2
    TCNT2 = 0;
    TCCR2 = 0x47; // WGM mode 2 -- CTC (reset on match); clock source prescaler/1024
    OCR2 = 249;   // for a 20MHz clock, 20 / 1024 / 250 -> 78.125 Hz timer interrupt
    TIFR |= 0x80; // clear interrupt flag
    TIMSK |= 0x80; // enable timer 2 interrupt
    ASSR = 0; // disable asynchronous operation (external counts)
}

ISR(TIMER2_COMP_vect)
{
    adjust_smps();

    // increment 'ticks'; after 78.125 ticks, one second has elapsed.
    // We manage the fractional part by bumping the second display at
    // 78 ticks, but keeping a fractional part that's incremented
    // on each second. When the fractional part reaches 7, we wait an
    // extra tick to bump the second count.
    current_time.ticks++;

    if (current_time.ticks == 78) {

        one_second();

        current_time.ticks = 0;
        /*
        current_time.correction++;
        if (current_time.correction == 8) {
            current_time.correction = 0;
            current_time.ticks = 0xFF; // take an extra tick next second
        }
        */
    }

    check_buttons();
}

void one_second()
{
    current_time.S++;
    if (current_time.S == 60) {
        current_time.S = 0;
        current_time.M++;
    }
    if (current_time.M == 60) {
        current_time.M = 0;
        current_time.H++;
    }
    if (current_time.H == 24) {
        current_time.H = 0;
    }

    write_display();
}

void init_display()
{
    // PC3 is latch (active low), PC4 is data, PC5 is clock: 0x38 mask
    // set latch (active low) output high for now; leave clock and data low
    PORTC = (PORTC & ~0x38) | 0x08;
    // all three lines are outputs
    DDRC |= 0x38;
}

#define nop asm volatile("nop")

#define pulse_nop \
  nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; \
  nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; \
  nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; \
  nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; \
  nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; \
  nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; \
  nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; \
  nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop;

void write_digit(char digit)
{
    char i;
    // write 10 bits out; bit `digit` is low, rest are high
    for (i = 0; i < 10; i++) {
        if (digit == i)
            PORTC |= 0x10; // set data high
        else
            PORTC &= ~0x10; // set data low

        PORTC |= 0x20; // set clock high
        pulse_nop;
        PORTC &= ~0x20; // set clock low
        pulse_nop;
    }
}

char lookup_ones[] = {
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9,
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9,
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9,
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9,
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9,
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, };
char lookup_tens[] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    5, 5, 5, 5, 5, 5, 5, 5, 5, 5, };

void write_display()
{
    write_digit(lookup_ones[current_time.S]);
    //write_digit(lookup_tens[current_time.S]);
    //write_digit(lookup_ones[current_time.M]);
    //write_digit(lookup_tens[current_time.M]);
    //write_digit(lookup_ones[current_time.H]);
    //write_digit(lookup_tens[current_time.H]);

    // clock the latch to load new values into shift register outputs
    PORTC &= ~0x08; // active-low: set low
    pulse_nop;
    PORTC |=  0x08; // return high
    pulse_nop;
}

void init_buttons()
{
    DDRD &= ~0x07; // PD0, PD1, PD2 are inputs (H, M, S)

    debounce_H = debounce_M = debounce_S = 0;
    registered_H = registered_M = registered_S = 0;
}

void check_buttons()
{
    return;

    // check once per 78.125Hz tick. Three ticks with a '1' is sufficiently debounced
    // to register a button-press.
    
    if ((PIND & 0x01) && debounce_H < 3) // H button
        debounce_H++;
    else {
        debounce_H = 0;
        registered_H = 0;
    }
    if ((PIND & 0x02) && debounce_M < 3) // M button
        debounce_M++;
    else {
        debounce_M = 0;
        registered_M = 0;
    }
    if ((PIND & 0x04) && debounce_S < 3) // S-clear button
        debounce_S++;
    else {
        debounce_S = 0;
        registered_S = 0;
    }

    if (debounce_H == 3 && !registered_H) {
        current_time.H++;
        if (current_time.H == 24) current_time.H = 0;
        registered_H = 1;
    }
    if (debounce_M == 3 && !registered_M) {
        current_time.M++;
        if (current_time.M == 60) current_time.M = 0;
        registered_M = 1;
    }
    if (debounce_S == 3 && !registered_S) {
        current_time.S = 0;
        registered_S = 1;
    }
}

static void init_smps()
{
    // set up PWM output to power MOSFET, initially with duty-cycle 0 (no switching)
    
    TCNT1 = 0;
    OCR1A = 0;      // initial 16-bit PWM value
    TCCR1A = 0x81; // COM1A[1:0] = 0b10 (set OC1A on compare match), WGM1[1:0] = 0b11 (phase/freq-correct 8-bit PWM)
    TCCR1B = 0x01; // WGM1[3:2] = 0b00 (see above), CS1[2:0] = clk

    // set up OC1A (PB1) output, low by default until timer is set up
    PORTB &= ~2;
    DDRB |= 2;

    // set up ADC on voltage feedback pin
    ADMUX = 0xe0;  // 2.56V internal Vref, ADC0 (PC0) input, left-adjust result
    ADCSRA = 0xe7; // enable, start conversion, free-running, interrupts disabled, prescaler at clk/128

    OCR1A = 0xc0;
}

char __level = 0;

void adjust_smps()
{
    return;

    char val = ADCH;
    char target = 170; // 170 volts (voltage divider is 100:1; units of ADCH with 2.56V ref are 0.01V

    if (val > 190) { // safety shutoff at 190V
        OCR1A = 0;
    }
    if (val < target) {
        if (OCR1A < 0xa0) OCR1A++;
    }
    if (val > target) {
        if (OCR1A > 0) OCR1A--;
    }
}

int main()
{
    init_time(); // initialize the time
    init_display();
    init_buttons();
    init_smps();
    sei(); // enable interrupts

    while (1);
}
