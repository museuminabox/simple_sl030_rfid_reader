# Simple SL030 RFID reader

Basic code to read IDs from RFID tags using an SL030 reader on a Raspberry Pi

Tries for a few seconds to read a tag on the reader.  If a tag is found, its ID is printed to stdout in hex.

## Usage

 1. Install Mike McCauley's [C library for Broadcom BCM 2835](http://www.airspayce.com/mikem/bcm2835/index.html) (used to access the I2C bus on the Raspberry Pi)
 1. Compile the code:
```
    gcc -static rfid_sl030.c -lbcm2835 -o rfid_sl030
```
 1. Run the code, as root
```
    sudo ./rfid_sl030
```

You might need to set the baudrate for the I2C bus to get it to work reliably:
```
    sudo modprobe -r i2c_bcm2708
    sudo modprobe i2c_bcm2708 baudrate=200000
```

## Files

 * [rfid_sl030.c](rfid_sl030.c) - Check for an RFID card on the reader, and if one is present output its ID number to stdout, in hex.
 * [rfid_read_ultralight.c](rfid_read_ultralight.c) - Read the contents of a Mifare Ultralight (or NTAG2xx card) and output it to stdout.  Assumes there is already a tag present, and that it's an Ultralight (or compatible) type.
 * [get_first_ndef_text_record.c](get_first_ndef_text_record.c) - Read the contents of a Mifare Ultralight/NTAG2xx tag, parse any contained NDEF records and output the first text record to stdout.
