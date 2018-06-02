#!/bin/bash

CROSS=arm-none-eabi-
ARM_CPU=-mcpu=arm1176jzf-s -fpic -ffreestanding
CC="$CROSS"gcc

CFLAGS=-std=gnu99 -O2 -Wall -Wextra
LFLAGS=-ffreestanding -O2 -nostdlib

$CC $ARM_CPU -c -o boot.o boot.s
$CC $ARM_CPU -c -o start.o start.c $CFLAGS

$CC -T linker.ld $LFLAGS -o kImg  boot.o start.o

qemu-system-arm -m 256 -M raspi2 -serial stdio -kernel kImg

