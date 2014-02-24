/*
 * Nixie Clock
 *
 * Copyright (c) 2014 Chris Fallin <cfallin@c1f.net>
 * Placed under the GNU GPL v2.
 */

/* 
 * Assume an ATmega16.
 *
 * IO connections (manufactured board version):
 *
 * PD0..PD3 are BCD for H-tens digit.
 * PD4..PD7 are BCD for H-ones digit.
 * PC0..PC3 are BCD for M-tens digit.
 * PC4..PC7 are BCD for M-ones digit.
 * PA0..PA3 are BCD for S-tens digit.
 * PA4,PA5,PA6,PB4 are BCD for S-ones digit.
 *
 * PB3/OC0 is gate for power MOSFET on high-voltage boost converter.
 *
 * PA7 is analog feedback on high-voltage bus (1/101 resistive prescaler).
 *
 * PB0, PB1, PB2 are pushbuttons 1, 2, 3.
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
        current_time.correction++;
        if (current_time.correction == 8) {
            current_time.correction = 0;
            current_time.ticks = 0xFF; // take an extra tick next second
        }
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
    // PD0..7, PC0..7, PA0..6, PB4 are outputs.
    // Start out with 0b1111 on all digits (digit blanked).
    PORTD = 0xff;
    DDRD = 0xff;
    PORTC = 0xff;
    DDRC = 0xff;
    PORTA |= 0x7f;
    DDRA |= 0x7f;
    PORTB |= 0x10;
    DDRB |= 0x10;
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
    // Hours and minutes are straight BCD out to the decoder/driver chips.
    // Note that ones digit of each is on high nibble rather than low nibble.
    PORTD = (lookup_tens[current_time.H]) | (lookup_ones[current_time.H] << 4);
    PORTC = (lookup_tens[current_time.M]) | (lookup_ones[current_time.M] << 4);

    // Seconds output has MSB on PB4 rather than PA7.
    uint8_t s = (lookup_tens[current_time.M]) | (lookup_ones[current_time.M] << 4);
    PORTA = (PORTA & 0x80) | (s & 0x7f);
    PORTB = (PORTB & 0xef) | ((s & 0x80) >> 3);
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
    
    if ((PINB & 0x01) && debounce_H < 3) // H button
        debounce_H++;
    else {
        debounce_H = 0;
        registered_H = 0;
    }
    if ((PINB & 0x02) && debounce_M < 3) // M button
        debounce_M++;
    else {
        debounce_M = 0;
        registered_M = 0;
    }
    if ((PINB & 0x04) && debounce_S < 3) // S-clear button
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
    //TCCR0 = (1<<WGM00) | (1<<WGM01) | (1<<COM01) | (1<<CS00);
    //OCR0 = 0x40;

    // set up OC0 (PB3) output, low by default until timer is set up
    PORTB &= ~8;
    DDRB |= 8;

    DDRA = 0xff;
    PORTA = 0;

    while (1) {
        PORTB |= 0x08;
        PORTB &= ~0x08;
        PORTA |= 1;
        PORTA &= ~1;
        asm volatile("nop; nop");
    }

    // set up ADC on voltage feedback pin
    ADMUX = 0xe0;  // 2.56V internal Vref, ADC0 (PC0) input, left-adjust result
    ADCSRA = 0xe7; // enable, start conversion, free-running, interrupts disabled, prescaler at clk/128

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
    DDRB = 1;
    PORTB = 0;
    while(1) { PORTB ^= 1; }
    //init_time(); // initialize the time
    //init_display();
    //init_buttons();
    init_smps();
#if 1
    sei(); // enable interrupts
    while (1);

#else
    one_second();
    one_second();
    one_second();
    current_time.S = 9;

    while(1) write_display();
#endif
}
