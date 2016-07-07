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

* PCJR_E.ROM : PCjr cartridge E0000h-EFFFFh
* PCJR_F.ROM : PCjr cartridge F0000h-FFFFFh

## ROM Image (Original PCjr Mode) ##

This program loads the following ROM image file from current working directory in original PCjr mode.

* bios.rom : F0000h-FFFFFh

## ROM Image (Cartridges) ##

Cartridge ROM image can be loaded with -d0, -d8, -e0, -e8, -f0 or -f8 option. The option name means starting address:

* -d0 for D0000h, application
* -d8 for D8000h, application, 32KiB extended video (Kakucho-hyoji) mode cartridge
* -e0 for E0000h, application for PCjr
* -e8 for E8000h, application for PCjr, 32KiB cartridge BASIC for PCjr
* -f0 for F0000h, system
* -f8 for F8000h, system

Maximum ROM image size is 96KiB.

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

./5511emu [-w] [-j] [-o] [-e] [-f] [-m RAM-size] [-d0 cartridge-d0] [-d8 cartridge-d8] [-e0 cartridge-e0] [-e8 cartridge-e8] [-f0 cartridge-f0] [-f8 cartridge-f8] [A drive image [B drive image [C drive image [D drive image]]]]
```

### Options ###

* -w: warm boot
* -j: PCjr cartridge
* -o: Original PCjr mode
* -e: Set window size for extended video mode
* -f: Fast mode - CPU 7.2MHz, like JX-5 extended (kakucho) mode
* -m 64: RAM size 64KiB (no memory expansion)
* -m 128: RAM size 128KiB (64KB memory expansion)
* -m 256: RAM size 256KiB (64KB memory expansion + 1 x 128KB memory expansion)
* -m 384: RAM size 384KiB (64KB memory expansion + 2 x 128KB memory expansion)
* -m 512: RAM size 512KiB (64KB memory expansion + 3 x 128KB memory expansion)
* -m 640: RAM size 640KiB (64KB memory expansion + 4 x 128KB memory expansion) (Original PCjr mode only)
* -d0 cartridge-d0: Load cartridge image from address D0000h
* -d8 cartridge-d8: Load cartridge image from address D8000h
* -e0 cartridge-e0: Load cartridge image from address E0000h
* -e8 cartridge-e8: Load cartridge image from address E8000h
* -f0 cartridge-f0: Load cartridge image from address F0000h
* -f8 cartridge-f8: Load cartridge image from address F8000h

### Note ###

Default RAM size is 640KiB for original PCjr mode or 512KiB. Please note that the main and 64KB memory expansion RAM which is also used as VRAM is slower than 128K memory expansion RAM. If the fast memory is located at the lowest memory address, memory size difference may cause program execution speed difference. In PCjr mode the slow memory is always located at the lowest memory address.

## Limitations ##

* The RAM size 64KiB option (-m 64) just sets the 8255 PC3 bit. It does not hide the 128KiB RAM.
* Floppy disk images cannot be changed while the emulator is running.
* There are bugs in CPU emulation, such as incorrect instruction clock cycles, rep prefix with other prefixes, etc.
* Superfast and quiet diskette drives.
* CG1 (Character Generator 1) uses CG2 fonts, because CG1 ROM is not readable from CPU and ROM reader is needed to create CG1 ROM image file.
* Cassette and parallel port are not supported.
* Serial port emulation is not implemented. No error is reported by BIOS since it is an expansion card on JX. In original PCjr mode the BIOS may report an error.
* BIOS continues booting when a kj-rom checksum test fails because HLT instruction is not properly emulated.
* Joystick is emulated as not connected.