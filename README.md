# IRem
IRem is a USB device that allows you to control your computer with an Infrared remote controller.
It uses Attiny85 microcontroller and vusb, a great library for most AVR's that provides sotware USB communication,
so there is no extra hardware to manage USB connection.

Features:

*No drivers needed. (Connects as HID Device)

*NEC IR protocol support.

*Mice and Keyboard control.

*Implementation based on interrupts.

*Tested on Linux, minor changes will make it run on windows too. (will be implemented soon)



To do:

*CODE CLEANUP. (Separate files for decoder and usb/main program)

*CODE OPTIMIZATION.

*Add more IR protocols.

*Add the capability of using other IR remotes. (Auto-program new keys on the run)



You can use this code for your own DIY remote, the code is provided as-is, with any kind of warranty or support,
but I hope you can use it, or make it fit your needs.


******************** YOU CAN`T USE THIS CODE FOR COMMERCIAL BENEFIT ******************** 
