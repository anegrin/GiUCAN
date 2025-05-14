#!/bin/sh
rm -rf dist
rm -rf build
mkdir -p dist
mkdir -p build
CFLAGS="-DECHO_MODE -DDEBUG_MODE" make clean all;mv build/GiUCAN*.elf dist/GiUCAN_ECHO_MODE.elf
CFLAGS="-DSLCAN" make clean all;mv build/GiUCAN*.elf dist/GiUCAN_SLCAN.elf
