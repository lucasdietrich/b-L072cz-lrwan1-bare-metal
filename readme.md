# b-l072cz-lrwan1

## Prerequisites

- [GNU ARM Embedded Toolchain](https://developer.arm.com/open-source/gnu-toolchain/gnu-rm/downloads)

## Usage

Build with `make`
Flash with `./flash.sh`
Monitor serial with `./monitor.sh`
Disassemble with `./dis.sh`
Debug in VSCode with `./debug.sh`

## Notes

Read user data structure from flash with GDB: `x /32wx 0x0802FD00`