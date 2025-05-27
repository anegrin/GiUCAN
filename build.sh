#!/bin/sh
rm -rf dist
rm -rf build
mkdir -p dist
mkdir -p build
CFLAGS="-DSLCAN" make clean all;mv build/GiUCAN*.elf dist/GiUCAN_SLCAN.elf
CFLAGS="-DC1CAN" make clean all;mv build/GiUCAN*.elf dist/GiUCAN_C1CAN.elf
CFLAGS="-DBHCAN" make clean all;mv build/GiUCAN*.elf dist/GiUCAN_BHCAN.elf
