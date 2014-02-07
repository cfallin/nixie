These are the raw Eagle schematic/PCB design files and the AVR firmware for a
Nixie tube clock.

Nixie tubes are gas-discharge neon display tubes. They take ~150 volts at a few
milliamps and glow a soft, pleasing orange. Each digit is a separate cathode in
a particular shape, so the numerals look more natural than more contemprary
7-segment display designs.

This project is a small microcontroller-based digital clock using 6 nixie tubes
(for HH:MM:SS). An ATmega16 8-bit AVR microcontroller keeps time based on its
20MHz system clock provided by a crystal oscillator, and controls six digit
driver ICs, one per Nixie tube. Each driver has high-voltage open-collector
outputs capable of directly interfacing with the display tube cathodes. The 150
volt anode supply is provided by a small switched-inductor boost converter,
which uses a power MOSFET driven by the microcontroller's PWM generator. The
design includes a feedback loop to an analog-to-digital (ADC) input on the AVR
so that it can regulate the tube voltage precisely.

Nixies can be found in various places online and often come from Russia. I
found mine on Etsy. Prices vary but I think the retro glow is well worth the
cost :-) The 74141 Nixie driver chips are also somewhat rare (e.g., not stocked
by Digikey) but can be found on eBay or at other small shops online.

=== DEVELOPMENT ENVIRONMENT ===

The firmware is written in C to be cross-compiled by avr-gcc. It assumes the
standard AVR headers that come with that compiler. The Makefile includes a
target to program the flash and fuses on the AVR using avrdude, but other
programmers can also be used.

The schematic and board were drawn in Eagle CAD. The additional libraries used
are included here.

=== IN PROGRESS ===

Work is currently in progress; in particular, the firmware is specific to an
earlier (hand-built) prototype version of the hardware and DOES NOT MATCH the
board. The board is currently (Feb 2014) out for a prototype run; when I build
the hardware designed here I'll update the firmware as appropriate.

=== LICENSE ===

All code, schematics and board design files here are Copyright (c) 2014 Chris
Fallin <cfallin@c1f.net>. Provided under the GNU GPL v2.
