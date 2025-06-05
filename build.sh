#!/bin/sh
rm -rf dist
rm -rf build
mkdir -p dist
mkdir -p build
CFLAGS="-DSLCAN -DINCLUDE_USER_CONFIG_H" make clean all;mv build/GiUCAN*.elf dist/GiUCAN_SLCAN.elf
CFLAGS="-DC1CAN -DINCLUDE_USER_CONFIG_H" make clean all;mv build/GiUCAN*.elf dist/GiUCAN_C1CAN.elf
CFLAGS="-DBHCAN -DINCLUDE_USER_CONFIG_H" make clean all;mv build/GiUCAN*.elf dist/GiUCAN_BHCAN.elf
