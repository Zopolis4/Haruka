# 5511emu #

This is an IBM JX emulator.

Default is JX mode which is for Japanese JX emulation. Original PCjr mode is expected to be similar to JX of Australian and New Zealand models.

## Environment ##

* GNU/Linux
* SDL

## ROM Images (JX Mode, Japanese) ##

This program loads following ROM image files from current working directory.

* BASE_E.ROM : E0000h-EFFFFh
* BASE_F.ROM : F0000h-FFFFFh
* FONT_8.ROM : Font ROM 80000h-8FFFFh
* FONT_9.ROM : Font ROM 90000h-9FFFFh
* FONT_A.ROM : Font ROM A0000h-AFFFFh
* FONT_B.ROM : Font ROM B0000h-BFFFFh

This program loads following ROM image files if -j option is specified.

* PCJR_D.ROM : PCjr cartridge D0000h-DFFFFh
* PCJR_E.ROM : PCjr cartridge E0000h-EFFFFh
* PCJR_F.ROM : PCjr cartridge F0000h-FFFFFh

## ROM Image (Original PCjr Mode) ##

This program loads the following ROM image file from current working directory in original PCjr mode.

* bios.rom : F0000h-FFFFFh

## Floppy disk images ##

Raw image is supported.

1024 byte sectors are optionally accessible to support copy protection. You need following files:

* File name: ("%s.%03d", original_image_name, (cylinder * 2 + head)) in printf format
* Contents: raw image of 1024 byte sectors in that track (cylinder and head)

For example, the original image name is "King's Quest.FDD" and cylinder 0 head 1 contains 5 1024-byte-sectors, the 5120 byte data will be loaded from file "King's Quest.FDD.001".

## Build ##

Run make command on GNU/Linux environment.

## Run ##

Run 5511emu.

```
#!sh

./5511emu [-w] [-j] [-o] [A drive image [B drive image [C drive image [D drive image]]]]
```

### Options ###

* -w: warm boot
* -j: PCjr cartridge
* -o: Original PCjr mode

## Limitations ##

* Floppy disk images cannot be changed while the emulator is running.
* Extended (Kakucho-hyoji) mode is not supported.
* JX-5 7.2MHz mode is not supported.
* Border color is not displayed.
* There are bugs in CPU emulation, such as incorrect instruction clock cycles, rep prefix with other prefixes, etc.
* Superfast and quiet diskette drives.
* CG1 (Character Generator 1) uses CG2 fonts, because CG1 ROM is not readable from CPU and ROM reader is needed to create CG1 ROM image file.
* Cassette and parallel port are not supported. Startup BIOS tests of them fail and "ERROR C J" or "ERROR C J H" is displayed. Press return key to continue booting.
* "ERROR H" is diskette error. The error is not displayed on warm boot. Probably there is a bug in diskette controller emulation.
* Serial port emulation is not implemented. No error is reported by BIOS since it is an expansion card on JX. In original PCjr mode the BIOS may report an error.
* BIOS continues booting when a kj-rom checksum test fails because HLT instruction is not properly emulated.
* Joystick is emulated but not controllable.
* Application cartridge support is not yet implemented.