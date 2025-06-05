#!/bin/sh
rm -rf dist
rm -rf build
mkdir -p dist
mkdir -p build

EXTRA_FLAG=""
[ -f inc/user_config.h ] && EXTRA_FLAG="-DINCLUDE_USER_CONFIG_H"

CFLAGS="-DSLCAN $EXTRA_FLAG" make clean all;mv build/GiUCAN*.elf dist/GiUCAN_SLCAN.elf
CFLAGS="-DC1CAN $EXTRA_FLAG" make clean all;mv build/GiUCAN*.elf dist/GiUCAN_C1CAN.elf
CFLAGS="-DBHCAN $EXTRA_FLAG" make clean all;mv build/GiUCAN*.elf dist/GiUCAN_BHCAN.elf
