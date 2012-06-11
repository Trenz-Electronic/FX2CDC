# FX2 firmware for Virtual COM example.
This repository contains virtual COM example project relevant to Trenz Electronic modules equipped with a USB microcontroller.
This project based on Cypress AN58764.

## Build project

* Download Cypress AN58764 from http://www.cypress.com/?rID=40248
* Replace vcp\Virtual COM Example Code\Virtual.c to Virtual.c from this repository
* Open and build project

## Driver
Project use example virtual COM port driver from AN58764.

## FX2 Firmware
FX2 firmware file in iic format can be found in firmware folder. To update FX2 
firmware on your module:

* Put EEPROM switch on your module to "OFF" state.
* Connect module to PC, using USB cable.
* Install Cypress generic driver if needed.
* Put EEPROM switch on your module to "ON" state.
* Run Cypress USB Console.
* Go Options->EZ-USB Interface
* Press "Lg EEPROM" button
* Select VirtualCom.iic from firmware folder
* Wait till operation completed
* Reconnect module from USB
* Install Cypress Virtual COM port driver if needed

## Detailed description
From FPGA side FX2 microcontroller configured to use Slave FIFO interface. Data
from PC is going to EP1OUT then reloaded to EP2 FIFO. Data from FPGA should be 
writed to EP8 FIFO. Flags
pins used to show state of this FIFO:

* Flag A - EP2 Empty Flag
* Flag B - EP8 Full Flag
* Flag C - EP8 Programmable Full Flag (show half of the buffer level)
* Flag D - Receive indicator - this flag toggle each time FX2 receive byte from PC

All signals configured to active high level.
