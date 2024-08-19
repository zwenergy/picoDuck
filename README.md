# picoDuck
An open-source RP2040-based flash cart for the Mega Duck aka Cougar Boy.
The RP2040 acts as the ROM chip of the cart.

## Software Side
In order to load a game onto the cartridge, you first have a "convert" a ROM to a UF2 file.

**Requirements:**
1. Python
2. CMake
3. RP2040 C SDK installed and paths set up

### Single ROM
Steps:

1. Use the bin2c.py script to convert a ROM file to a C-array. Usage: `bin2c.py ROMFILE rom.h`. Place the genereated `rom.h` file in the root of code directory, beside main.c.
2. Run `make` in the code directory.
3. Connect the Raspberry Pi Pico to the computer while holding down the BOOTSEL button.
4. Drag and drop the newly generated .uf2 file onto the Pico.

## Hardware Side
### BOM
| **Reference** | **Value**| **Links**
|---------------|----------|----------|
| U2, U3, U4, U5 | TXB0108PWR |[LCSC](https://www.lcsc.com/product-detail/Translators-Level-Shifters_Texas-Instruments-TXB0108PWR_C53406.html)|
| U6 | Raspberry Pi Pico ||
| Q1 | N-Channel MOSFET (SOT-23) |[LCSC](https://www.lcsc.com/product-detail/MOSFETs_Jiangsu-Changjing-Electronics-Technology-Co-Ltd-CJ2302_C2910175.html)|
| R1 | 1 kOhm resistor (0805) |[LCSC](https://www.lcsc.com/product-detail/Chip-Resistor-Surface-Mount_UNI-ROYAL-Uniroyal-Elec-0805W8F1001T5E_C17513.html)|
| R2 | 470 kOhm resistor (0805) |[LCSC](https://www.lcsc.com/product-detail/Chip-Resistor-Surface-Mount_UNI-ROYAL-Uniroyal-Elec-0805W8F4703T5E_C17709.html)|

### PCB
The PCB can be ordered using the Gerber files. A width of **1.2 mm** should be chosen with ENIG surface.

## Shell
A 3D printable shell for the cart can be found in the folder "shell".

## Disclaimer
**Use the files and/or schematics to build your own board at your own risk**.
This board works fine for me, but it's a simple hobby project, so there is no liability for errors in the schematics and/or board files.
**Use at your own risk**.
