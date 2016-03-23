#!/bin/bash
# Script de hola mundo
avr-gcc -I -Os -Wall -mmcu=atmega328p IRFemDecoder.h IRFemDecoder.c -o IRFemDecoder.elf

avr-objcopy -j .text -j .data -O ihex IRFemDecoder.elf IRFemDecoder.hex

avrdude -P /dev/ttyACM0 -b 19200 -c avrisp -p m328p -v -e -U lfuse:w:0xff:m -U hfuse:w:0xD1:m -U efuse:w:0x05:m -U flash:w:IRFemDecoder.hex
