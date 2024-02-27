# PCZSM DOS ZSM player

This is a DOS based player of the [ZSM format](https://github.com/ZeroByteOrg/zsound/blob/main/doc/ZSM%20Format%20Specification.md) originally targeted for the
[Commander X16](https://www.commanderx16.com/) platform.

The main purpose is to provide an example of how to use some of the features available on the [TexElec](https://texelec.com/) [SAAYM](https://texelec.com/product/saaym/) sound card which uses a [Yamaha YM2151](https://en.wikipedia.org/wiki/Yamaha_YM2151) and two [Philips SAA1099](https://en.wikipedia.org/wiki/Philips_SAA1099) sound chips.

This includes:

* YM2151 detection.
* YM2151 clock speed detection.
* YM2151 timer-based playback via assigned IRQ.

## SAAYM environment variable

If the SAAYM environment variable is defined, the player will use the settings defined there.

SET SAAYM=A210 I5 Y0 S0

* **A** - This is the address of where the SAAYM is configured for.
* **I** - This is the IRQ the SAAYM uses (if jumpered).  If there is no IRQ defined, the system timer will be used.
* **Y** - 0 is for the YM2151 clocked at 3.579545 MHz, 1 is for the YM2151 clocked at 4 MHz.
* **S** - 0 is for the SAA1099s clocked at 7.15909 MHz (Game Blaster), 1 is for the SAA1099s clocked at 8 MHz.

## Current Missing Features

* YM2151 clocked at 4 MHz frequency conversion.
* SAA1099 clocked at 8 MHz frequency conversion.
* VERA PCM playback.
* VERA PSG Pulse Width (SAA1099 natively lacks this feature).
* VERA channels; only the first twelve VERA channels are handled on the dual SAA1099 chips.

## Requirements
IBM PC or compatible
MS-DOS 3.0 or higher
8088/8086 CPU @ 4.77 MHz or higher (for PCZSM16)
80386 or higher CPU (for PCZSM32)
256 KB RAM

## Usage
PCZSM _**[options]**_ FILENAME.ZSM

Where options can be:
* `-rN` Repeat N times (if the ZSM repeats). -r only to repeat forever.

## Building the source

The source is intended to be built with the [Open Watcom](https://openwatcom.org/) compiler, though it may build with other compilers that can generate a DOS executable.

In the build output directory of choice type:
wmake -f _**[path_to_makefile]**_ _**[target]**_

Where _**target**_ can be:

| Target           | Configuration   | Platform          |
|------------------|-----------------|-------------------|
| all              | RELEASE & DEBUG | 32-bit and 16-bit |
| all-32           | RELEASE & DEBUG | 32-bit            |
| all-16           | RELEASE & DEBUG | 16-bit            |
| release          | RELEASE & DEBUG | 32-bit and 16-bit |
| release-32       | RELEASE         | 32-bit            |
| release-16       | RELEASE         | 16-bit            |
| debug            | RELEASE & DEBUG | 32-bit and 16-bit |
| debug-32         | DEBUG           | 32-bit            |
| debug-16         | DEBUG           | 16-bit            |
| clean            | RELEASE & DEBUG | 32-bit and 16-bit |
| clean-release    | RELEASE & DEBUG | 32-bit and 16-bit |
| clean-release-32 | RELEASE         | 32-bit            |
| clean-release-16 | RELEASE         | 16-bit            |
| clean-debug      | RELEASE & DEBUG | 32-bit and 16-bit |
| clean-debug-32   | DEBUG           | 32-bit            |
| clean-debug-16   | DEBUG           | 16-bit            |


## Coding Standards

Attempts to adhere to the [BARR-C:2018](https://barrgroup.com/embedded-systems/books/embedded-c-coding-standard) Embedded C Coding Standard.
