# Simple SL030 RFID reader

Basic code to read IDs from RFID tags using an SL030 reader on a Raspberry Pi

Tries for a few seconds to read a tag on the reader.  If a tag is found, its ID is printed to stdout in hex.

## Usage

 1. Install Mike McCauley's [C library for Broadcom BCM 2835](http://www.airspayce.com/mikem/bcm2835/index.html) (used to access the I2C bus on the Raspberry Pi)
 1. Compile the code:
    gcc -static rfid_sl030.c -lbcm2835 -o rfid_sl030
 1. Run the code, as root
    sudo ./rfid_sl030

You might need to set the baudrate for the I2C bus to get it to work reliably:
    sudo modprobe -r i2c_bcm2708
    sudo modprobe i2c_bcm2708 baudrate=200000
