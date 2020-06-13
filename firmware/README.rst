Reflow Oven Controller Firmware
####################################

Overview
********

Uses the Zephyr framework to create a soldering reflow oven controller using a cheap toaster oven.

Reads thermocouple voltage from inside the oven and uses a simple PI control loop to regulate to the correct temperature.  The soldering profile information is supplied from a host PC over serial/USB.

Uses the STM32F103CB6 microcontroller populated on the PCB in the above hardware folder.

Requirements
************

The Zephyr framework should be installed somewhere that can be found on the system.

Building and Running
********************
Some convenience scripts are included for development

Build
===========

To build the project simply run from the firmware top folder:

   ./build.sh

Programming
=======

Programming is performed using the STM32 bootloader and a serial adapter.  With the serial adapter hooked up to the connector run:

   ./program.sh


Serial Interface
=======

To communicate over serial to the console with the same bootloader programming connection use:

   ./serial_comms.sh


