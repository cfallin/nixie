OBJS = main.o

.PHONY: all
all: dimmer.hex

CC = avr-gcc
LD = avr-ld
OBJCOPY = avr-objcopy
CFLAGS = -mmcu=atmega8 -O3

dimmer.hex: dimmer.out
	$(OBJCOPY) -R .eeprom -O ihex dimmer.out dimmer.hex

dimmer.out: $(OBJS)
	$(CC) $(CFLAGS) -o dimmer.out $(OBJS)
	
.PHONY: prog
prog: dimmer.hex
	uisp -dprog=dapa --erase
	uisp -dprog=dapa --upload if=dimmer.hex


.PHONY: prog_fuses
prog_fuses:
	#uisp -dlpt=0x378 -dprog=dapa --wr_fuse_l=0xef
	#uisp -dlpt=0x378 -dprog=dapa --wr_fuse_h=0xc9
	uisp -dprog=dapa --wr_fuse_l=0xef
	uisp -dprog=dapa --wr_fuse_h=0xc9

.PHONY: eeprom
eeprom:
	uisp -dprog=dapa --download --segment=eeprom of=eeprom.hex
	$(OBJCOPY) -I ihex -O binary eeprom.hex eeprom.bin

	
.PHONY: clean
clean:
	rm -f dimmer.hex dimmer.out *.o
