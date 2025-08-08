# Introduction
This firmware for the EMstat Pico can be used in combination with nrf52840 boards to transmit Open Circuit Potential readouts in real time, it uses the METHODsrcript library

# Requirements
Download the METHODscript library from the official GitHub repository https://github.com/PalmSens/MethodSCRIPT_Examples

# Board connections
nrf52840 - RX to EMstat Pico pin 3

nrf52840 - TX to EMstat Pico pin 2

sensor - lead 1 to EMstat Pico pin CE_0

sensor - lead 2 to EMstat Pico pin WE_0

sensor - lead 2 to EMstat Pico pin RE_0

# Setup
Step 1: Download Arduino IDE and install the nrf52 board pack

Step 2: Create a folder for "Potentiostat Bluetooth Firmware.ino" and copy: MathHelpers.h, MSComm.c, MSComm.h and MSCommon.h

Step 3: Upload the code

Step 4: Connect to the bluetooth and check MAC address https://learn.microsoft.com/en-us/answers/questions/3749375/how-do-i-find-a-connected-bluetooth-device-from-it

Step 5: In "Potentiostat Bluetooth Logger.py" copy the MAC address

Step 6: Run the python file, press "Y" to start measuring, the values will be displayed in real time

Step 7: To stop the program press "C" the values will be saved in a file


