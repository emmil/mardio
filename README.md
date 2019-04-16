# The MARDIO MP3 player

## About

Mardio (My own ARM Radio) is a bare metal application, for embedded devices. However it can run in Linux and other OSes as user application, too. Currently supported are STM32F4 Discovery board and Linux.

## Features

- Bare metal, requires no OS
- Small footprint (code and memory)
- Static memory allocation at start
- Extensible debug console
- Simple resource manager and FS

## Video presentation

https://www.youtube.com/watch?v=-wV5sX4FzDM&t

## Get the source code

https://github.com/emmil/mardio

## Build instructions

Depending on the target and host platform you will need different packages.

### Build as Linux user application

For building under Fedora make sure you have following packages installed
```
dnf install gcc SDL-devel libmad-devel
```

The build process is simple
```
make linux
```
A file named mardio.elf will be generated, that can be run as a user application.

### Build as native ARM STM32F4 application

Get Fedora running in QEMU. There is a howto on the Fedoraproject wiki
[HowToQemu]https://fedoraproject.org/wiki/Architectures/ARM/HowToQemu

Or have an ARM computer with Linux available (e.g. Raspberry Pi).

Although building with a cross compiler is an option and it works, I do not recommend it as compiler takes sometimes header files from the host OS.

Once the gcc and make commands work for you, run `make stm` to compile. No external dependencies should be needed.

The build process will generate `mardio.bin` file, which can be flashed directly into the STM32F4 Discovery board using e.g. StLink tool. 

The console output is redirected to USART3, with rate of 115200 bauds.

## Usage

Once executed, the boot process should display a message similar to the one below:
```
=== Init Start ===
io_init
    Input: 4 buffers, each 1024, total 4KB
    Output: 4 buffers, each 4608, total 18KB
fs_init
res_init: 2 resources found.
host_net_init
host_snd_init
   alsa version: 1.1.5
pl_init
mardio 0.1e
=== Init Done ===

]
```

The right squared bracket ` ] ` tells the application is ready to accept commands.

Please note, though it is possible to control the application though the console, the console is mainly for the debug purposes.

Below is a list of available commands:
- help - Display a list of known commands
- echo - Echoes console input to the output
- cls - Clear console scrollback
- play - Play a stream or a local MP3 or Wave file
- stop - Stop playing
- svol - Change sound volume
- mute - Mute sound volume
- radio - Play one of the pre selected internet MP3 streams
- io - Show state of input and output circular buffers
- net - Show state of the network connection
- snd - Show state of the sound module
- pls - Show state of the player module
- rls - Show list of embedded resource files
- ver - Print version
- exit - Exit to OS (Linux only)

## Source code overview

### Directories

A short overview of content each individual directories
- common - Core of the application, used by all supported platforms
- libs/id3tag - routines from id3tag library, used by all supported platforms
- libs/libc - libc routines, used by all supported platforms
- null - Template files for porting to new platform
- linux - OS Linux specific files
- libs/artwork - GUI resources in form of a BMP picture, used by Linux only
- stm32 - STM32F4 specific files
- libs/Audio - Audio driver for CS43L22 (STM32F4)
- libs/cmsis - Compatibility headers for ARM (STM32F4)
- libs/ff - FAT FS driver (STM32F4)
- libs/libmad-0.15.1b - the MP3 decoder (STM32F4)
- libs/lwip-2.03 - lightweight IP network stack (STM32F4)
- libs/st - linker script and startup files for STM32F4
- libs/STM32* - STM32F4 specific peripheral and protocol drivers

## License

Code from several open source project has been used. Images as well.
Below is a list with their licenses. Rest of the code is under GLP 3

ucLibs - LGPL v2.1
Quake3 - GNU GPL2
libid3tag - GNU GPL2
madlld - madlld license
libmad - GNU GPL2
ff - Free software
lwip - BSD license
libmad - GNU GPL2
pcd8544 code - Tilen Majerle GNU GPL 3
startup code, peripheral drivers - ST Liberty SW License 20 Jul 2011

MP3 logo - Wikipedia
USB logo - Wikipedia
Sand glass (clock136.svg) - from web page flaticon.com "designed by Freepik"
MC cassete (music46.svg) - from web page flaticon.com "designed by Freepik"
Radio (communication5.svg) - from web page flaticon.com "designed by Freepik"
Network (network19.svg) - from web page flaticon.com "designed by Freepik"
Speaker (speaker117.svg) - from web page flaticon.com "designed by Freepik"
STM32 logo belongs to STMicroelectronics

