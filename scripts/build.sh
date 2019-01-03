#!/bin/bash
#
#      This file is part of the KoraOS project.
#  Copyright (C) 2015-2019  <Fabien Bavent>
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
    qemu-system-i386 --cdrom KoraOs.iso --serial stdio --smp 2 -m 32
}

run_raspi2 () {
    qemu-system-arm -m 256 -M raspi2 -serial stdio -kernel ./bin/kImage
}

# -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
export iso_name=KoraOs.iso

build_raspi2 () {
    export target=arm-raspberry2-none
    case "`uname -m`"
    in
        raspi2) unset CROSS ;;
        *) export CROSS=arm-none-eabi- ;;
    esac

    make -f $SRC_KRN/Makefile kImage
}

build_x86 () {
    export target=x86-pc-none
    case "`uname -m`"
    in
        i386|i486|i686) unset CROSS ;;
        *) export CROSS=i386-elf- ;;
    esac

    make -f $SRC_KRN/Makefile kImage

    rm -rf iso
    mkdir -p iso/{etc,bin,boot,lib}
    mkdir -p iso/usr/{bin,include,lib,man}
    # mkdir -p iso/{dev,mnt,proc,sys,tmp}


    $SCRIPT_HOME/scripts/drivers.sh

    # Import files
    cp -v $SRC_KRN/bin/kImage iso/boot/kImage

    mkdir -p iso/boot/grub

    if [ -z $isomode ]
    then
        # Create ISO (Option 1)
        echo "    ISO $iso_name (option 1)"
        cp $SRC_KRN/scripts/cfg/grub.cfg iso/boot/grub/
        grub-mkrescue -o "$iso_name" iso
    else
        # Create ISO (Option 2)
        echo "    ISO $iso_name (option 2)"
        cp $SRC_KRN/scripts/cfg/stage2_eltorito iso/boot/grub/
        cp $SRC_KRN/scripts/cfg/menu.lst iso/boot/grub/
        genisoimage -R -b boot/grub/stage2_eltorito -no-emul-boot -boot-load-size 4 -boot-info-table -o "$iso_name" iso
    fi

    ls -lh "$iso_name"
}

clean () {
    make -f $SRC_KRN/Makefile distclean
}

all () {
    $0 clean build run
}
# -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

look_arch() {
    arch_="$1"
    case "$arch_"
    in
        i386|i486|i686) arch_='x86' ;;
        raspi2) arch_='raspi2' ;;
    esac
    echo "$arch_"
}


ARCH=`uname -m`
ARCH=`look_arch $ARCH`


while (( $# > 0 ))
do
    case "$1"
    in
        -m) export ARCH=`look_arch $2` ;;
        clean) $1 ;;
        build|run) "$1"_"$ARCH" ;;
        all) clean && "build_$ARCH" && "run_$ARCH" ;;
    esac
    shift
done
