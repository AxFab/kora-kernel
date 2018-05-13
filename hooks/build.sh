#!/bin/bash

export PATH="$HOME/opt/bin":$PATH
export CROSS=i686-elf-

require () {
    apt-get install -y binutils nasm xorriso
}

dbg () {
    kvm # Use serial, gdb localhost:1234, no screen
}

make_util () {
    if [ -f $SRC_UTL/src/$1.c ]; then
        ${CROSS}gcc -c -o $SRC_UTL/obj/$1.o $SRC_UTL/src/$1.c -nostdlib
        ${CROSS}ld -T $SRC_UTL/arch/x86/app.ld $SRC_UTL/obj/crt0.o $SRC_UTL/obj/$1.o -o $SRC_UTL/bin/$1
        cp $SRC_UTL/bin/$1 iso/bin/$1
    fi
}

# -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
x86 () {
    export SRC_KRN=.
    export SRC_UTL=../utilities
    export iso_name=KoraOs.iso
    # export target=x86-pc-smkos
    # export VERBOSE=1
    export NODEPS=1

    # make -f ../skc/Makefile
    # make -f ../skc/Makefile crt0
    # make -f ../Makefile
    make -f $SRC_KRN/Makefile kImg

    rm -rf iso

    # Import kernel and shell commands
    mkdir -p iso/{bin,boot,lib}
    cp $SRC_KRN/bin/kImg iso/boot/kImg
    # cp iso/kImg.map iso/boot/kImg.map
    # cp ../lib/* iso/lib/*

    if [ -f $SRC_UTL/arch/x86/crt0.asm ]; then
        mkdir -p $SRC_UTL/{obj,bin,lib}
        nasm -f elf32 -o $SRC_UTL/obj/crt0.o $SRC_UTL/arch/x86/crt0.asm
        make_util init
    fi
    size iso/bin/*

    cp -r $SRC_UTL/data iso/data

    # Import apps
    mkdir -p iso/usr/{bin,include,lib,man}

    # Empty dirs for mount point
    mkdir -p iso/{dev,mnt,proc,sys,tmp}

    # Create config
    mkdir -p iso/etc
    cat > iso/etc/mtab << EOF
sysfs /sys sysfs rw,nosuid,nodev,noexec
procfs /proc procfs rw,nosuid,nodev,noexec
devfs /dev devfs rw,nosuid,noexec
tmpfs /tmp tmpfs rw,nosuid
/dev/sda / gpt rw
/dev/sda1 /mnt/hdd1  rw
/dev/sdc /mnt/cdrom ro
EOF

  # Create grub config
    mkdir -p iso/boot/grub
    cat >  iso/boot/grub/grub.cfg << EOF
set default="0"
set timeout="0"

menuentry "Kora x86" {
  multiboot /boot/kImg
}
EOF

    grub-mkrescue -o "$iso_name" iso 2>/dev/null >/dev/null
    ls -lh "$iso_name"
}


# -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
export target=x86-pc-linux-gnu
$1
