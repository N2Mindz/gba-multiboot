## Project parts

* `sender`: Runs on the PC, communicates with Teensy over serial, most logic here, sends program to GBA.
* `teensy-passthrough`: Bitbanged SPI on the Teensy.
* `gba-hello-world`: Example hello world program.
* `recver`: Runs on the PC, communicates with Teensy over serial, receives serial output of uploaded program.

## Dependencies

* avr-gcc for Teensy passthrough
* devkitPro for hello world program

## Building

    cd sender
    ./build.sh
    cd ../recver
    ./build.sh
    cd ../teensy-passthrough
    make
    # Upload teensy-passthrough.hex to Teensy
    cd ../gba-hello-world
    make
    cd ..

## Example usage

    ./sender/sender ./gba-hello-world/gba-hello-world_mb.gba && ./recver/recver

![Picture](https://pbs.twimg.com/media/DGYNCVbXcAElCzw.jpg)

## Pin numbering

### GBA Link-Cable

     _________  
    / 6  4  2 \ 
    \_5_ 3 _1_/ 
        '-'

### GBA Socket

     ___________  
    |  2  4  6  | 
     \_1_ 3 _5_/ 
         '-'     

## Wiring

| Teensy Pin    | GBA Pin       | Purpose       | My Cable Colour |
| ------------- | ------------- | ------------- | --------------- |
| B0            | 5             | SC / CLK      | Red             |
| B1            | 3             | SI / MOSI     | White           |
| B2            | 2             | SO / MISO     | Black           |
| GND           | 6             | GND           | -               |

Note: The cable colours used by your cable are very likely different. If your cable doesn't have a GND pin, connecting to battery ground also works.

## Legal

To the extent possible under law, the author(s) have dedicated all copyright and related
and neighboring rights to this software to the public domain worldwide.

This software is distributed without any warranty.

You should have received a copy of the CC0 Public Domain Dedication along with this software.
If not, see <http://creativecommons.org/publicdomain/zero/1.0/>.
