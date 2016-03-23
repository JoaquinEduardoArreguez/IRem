#!/bin/bash
# Script de hola mundo
avr-gcc -I -Os -Wall -mmcu=atmega328p IRFemDecoder.h IRFemDecoderC.c -o IRFemDecoderC.elf

avr-objcopy -j .text -j .data -O ihex IRFemDecoderC.elf IRFemDecoderC.hex

avrdude -P /dev/ttyACM0 -b 19200 -c avrisp -p m328p -v -e -U lfuse:w:0xff:m -U hfuse:w:0xD1:m -U efuse:w:0x05:m -U flash:w:IRFemDecoderC.hex
