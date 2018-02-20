NAME: CSE 438 Assignment-3 Part-1
________________________________________________________________________________________________________________________________________

AUTHORS:  Team-1

Vishwakumar Dhaneshbhai Doshi (1213322381)

Nisarg Trivedi (1213314867)
________________________________________________________________________________________________________________________________________

INCLUDED IN REPOSITORY:

-> Ultra_display.c (Program Source code)

-> Uled.h (library)

-> Makefile (makefile for the sourcecode)

-> README

________________________________________________________________________________________________________________________________________

ABOUT: 

This project uses two devices. An SPI LED matrix and an Ultrasonic distance measuring sensor. Here the ultrasonic 
sends a trigger pulse at contnous time intervals which generate a sonic burst of 8 pulses. As the sonic burst starts, 
the Echo pulse rises and falls when all the sonic busrts are back. The size of the Echo pulse gives the distance 
between the object and the Ultrasonic sensor. The pattern to be diplayed  depends on the distance between the 
object and the sensor and the direction of the movement of the object and changes accordingly.
________________________________________________________________________________________________________________________________________

SYSTEM REQUIREMENTS:

-> Linux machine for host. Compilation must be done on this machine and not on the board.

-> LINUX KERNEL : Minimum version of 2.6.19 is expected.

-> SDK: iot-devkit-glibc-x86_64-image-full-i586-toolchain-1.7.2

-> GCC:  i586-poky-linux-gcc

-> Intel Galileo Gen2 board

-> Ultrasonic Sensor(HC-Sr04)

-> 8X8 Matrix LED display using MAX7219 Drivers 

-> Bread-board and wires
________________________________________________________________________________________________________________________________________

SETUP:

-> If your SDK is installed in other location than default one, change the path accordingly in makefile.

-> You must boot Galileo from SD card with linux version 3.19.

-> Open 2 terminal windows, one will be used for host commands and other will be used to send command to Galileo board.

-> Connect the FTDI cable and on one terminal type:

           sudo screen /dev/ttyUSB0 115200

 to communicate with board. This will be used for board comminication and is referred to as screen terminal, while other window is used for host commands. 

-> Connect ethernet cable and choose board ip address 192.168.1.100 and host ip address 192.168.1.105 (last number can be any from 0-255 but different from Host's number) by typing:

ifconfig enp2s0 192.168.1.105 netmask 255.255.0.0 up (on host terminal)

ifconfig enp0s20f6 192.168.1.100 netmask 255.255.0.0 up (on screen terminal)

-> Connect Ultrasonic sensor to the Board. The TRIGGER pin is to be connected to 2 and ECHO pin to 3 on Galileo board.

-> Connect LED Matrix. The chip select pin to IO12, Data to IO11 and SCLK to IO13.
________________________________________________________________________________________________________________________________________

COMPILATION:

-> On the host, extract all files in zip file in one directory, open directory in terminal in which files are present and type make.

-> To copy files to Galileo board type in host terminal:

    scp /home/vishva/gpio/Ultra_display root@192.168.1.100:/home/vishva (only an example use your user name and corresponding path name)
________________________________________________________________________________________________________________________________________

EXECUTION:

-> type in screen terminal:

    ./Ultra_display
________________________________________________________________________________________________________________________________________

EXPECTED OUTPUT:

-> On running the object code, the pattern will move faster or slower according to the distance of the object from the sensor and will change direction
according to the direction of movement of the object relative to sensor.


