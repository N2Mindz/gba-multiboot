## Project parts

* `sender`: Runs on the PC, communicates with Teensy over serial, most logic here.
* `teensy-passthrough`: Bitbanged SPI on the Teensy.
* `gba-hello-world`: Example hello world program.

## Dependencies

* avr-gcc for Teensy passthrough
* devkitPro for hello world program

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