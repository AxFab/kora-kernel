#!/bin/bash
#      This file is part of the KoraOS project.
#  Copyright (C) 2015  <Fabien Bavent>
#
#  This program is free software: you can redistribute it and/or modify
#  it under the terms of the GNU Affero General Public License as
#  published by the Free Software Foundation, either version 3 of the
#  License, or (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU Affero General Public License for more details.
#
#  You should have received a copy of the GNU Affero General Public License
#  along with this program.  If not, see <http://www.gnu.org/licenses/>.
# -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

SCRIPT_DIR=`dirname $BASH_SOURCE{0}`
SCRIPT_HOME=`readlink -f $SCRIPT_DIR/..`

export PATH="$HOME/opt/bin":$PATH
export SRC_KRN=$SCRIPT_HOME
export SRC_UTL=`readlink -f $SCRIPT_HOME/../utilities`
export NODEPS=1

# Here is the commands to run for Debian base setup
setup_deb() {
    apt-get update
    apt-get install -y binutils gcc nasm xorriso grub
    apt-get install -y qemu gdb
}

# make_util () {
#     if [ -f $SRC_UTL/src/$1.c ]; then
#         ${CROSS}gcc -c -o $SRC_UTL/obj/$1.o $SRC_UTL/src/$1.c -nostdlib
#         ${CROSS}ld -T $SRC_UTL/arch/x86/app.ld $SRC_UTL/obj/crt0.o $SRC_UTL/obj/$1.o -o $SRC_UTL/bin/$1
#         cp $SRC_UTL/bin/$1 iso/bin/$1
#     fi
# }

# -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
run_x86 () {
    x86
    qemu-system-i368 --cdrom KoraOs.iso --serial stdio --smp 2
}

run_raspberry-pi2 () {
    raspberry-pi2
    qemu-system-arm -m 256 -M raspi2 -serial stdio -kernel ./bin/kImage
}

# -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

raspberry-pi2 () {
    export target=arm-raspberry2-none
    export CROSS=arm-none-eabi-

    make -f $SRC_KRN/Makefile kImage
}

x86 () {
    export target=i386-pc-none
#    export CROSS=i686-elf-
    export iso_name=KoraOs.iso

    make -f $SRC_KRN/Makefile kImage

    rm -rf iso
    mkdir -p iso/{etc,bin,boot,lib}
    mkdir -p iso/usr/{bin,include,lib,man}
    # mkdir -p iso/{dev,mnt,proc,sys,tmp}

    # Import files
    cp $SRC_KRN/bin/kImage iso/boot/kImage
    # cp ../lib/* iso/lib/*
    # cp ../bin/* iso/bin/*
    # size iso/bin/*

    # Create grub config
    mkdir -p iso/boot/grub
    cat >  iso/boot/grub/grub.cfg << EOF
set default="0"
set timeout="0"

menuentry "Kora x86" {
  multiboot /boot/kImage
}
EOF

    grub-mkrescue -o "$iso_name" iso 2>/dev/null >/dev/null
    ls -lh "$iso_name"
}


# -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

$1
