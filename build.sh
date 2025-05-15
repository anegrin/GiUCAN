#!/bin/sh
rm -rf dist
rm -rf build
mkdir -p dist
mkdir -p build
CFLAGS="-DECHO_MODE -DDEBUG_MODE -DVERBOSE" make clean all;mv build/GiUCAN*.elf dist/GiUCAN_ECHO_MODE.elf
CFLAGS="-DSLCAN" make clean all;mv build/GiUCAN*.elf dist/GiUCAN_SLCAN_C1.elf
CFLAGS="-DSLCAN -DCAN_BITRATE=CAN_BITRATE_125K" make clean all;mv build/GiUCAN*.elf dist/GiUCAN_SLCAN_BH.elf
