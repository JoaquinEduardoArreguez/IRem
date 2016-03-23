# WinAVR cross-compiler toolchain is used here
CC = avr-gcc
OBJCOPY = avr-objcopy
DUDE = avrdude

# If you are not using ATtiny2313 and the USBtiny programmer, 
# update the lines below to match your configuration
CFLAGS = -Wall -Os -I usbdrv -mmcu=attiny85
OBJFLAGS = -j .text -j .data -O ihex
DUDEFLAGS = -P /dev/ttyACM0 -b 19200 -c avrisp -p t85 -v -e -U lfuse:w:0xce:m -U hfuse:w:0xdf:m -U efuse:w:0xff:m
#-U lfuse:w:0xce:m -U hfuse:w:0xdf:m -U efuse:w:0xff:m

# Object files for the firmware (usbdrv/oddebug.o not strictly needed I think)
OBJECTS = usbdrv/usbdrv.o usbdrv/oddebug.o usbdrv/usbdrvasm.o main.o

# Command-line client
#CMDLINE = usbtest.exe

# By default, build the firmware and command-line client, but do not flash
all: main.hex $(CMDLINE)

# With this, you can flash the firmware by just typing "make flash" on command-line
flash: main.hex
	$(DUDE) $(DUDEFLAGS) -U flash:w:$<

# One-liner to compile the command-line client from usbtest.c
#$(CMDLINE): usbtest.c
#	gcc -I ./libusb/include -L ./libusb/lib/gcc -O -Wall usbtest.c -o usbtest.exe -lusb

# Housekeeping if you want it
clean:
	$(RM) *.o *.hex *.elf usbdrv/*.o irfemdecoder/*.o

# From .elf file to .hex
%.hex: %.elf
	$(OBJCOPY) $(OBJFLAGS) $< $@

# Main.elf requires additional objects to the firmware, not just main.o
main.elf: $(OBJECTS)
	$(CC) $(CFLAGS) $(OBJECTS) -o $@

# Without this dependance, .o files will not be recompiled if you change 
# the config! I spent a few hours debugging because of this...
$(OBJECTS): usbdrv/usbconfig.h

# From C source to .o object file
%.o: %.c	
	$(CC) $(CFLAGS) -c $< -o $@

# From assembler source to .o object file
%.o: %.S
	$(CC) $(CFLAGS) -x assembler-with-cpp -c $< -o $@
