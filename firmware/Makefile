OBJS = main.o

.PHONY: all
all: nixie.hex

CC = avr-gcc
LD = avr-ld
OBJCOPY = avr-objcopy
CFLAGS = -mmcu=atmega16 -O3

nixie.hex: nixie.out
	$(OBJCOPY) -R .eeprom -O ihex nixie.out nixie.hex

nixie.out: $(OBJS)
	$(CC) $(CFLAGS) -o nixie.out $(OBJS)
	
.PHONY: prog
prog: nixie.hex
	avrdude -c usbtiny -F -p m16 -U flash:w:nixie.hex


.PHONY: prog_fuses
prog_fuses:
	#avrdude -c usbtiny -F -p m16 -U lfuse:w:0xd0:m -U hfuse:w:0x99:m
	avrdude -c usbtiny -F -p m16 -U lfuse:w:0xd0:m -U hfuse:w:0x99:m

.PHONY: clean
clean:
	rm -f nixie.hex nixie.out *.o
