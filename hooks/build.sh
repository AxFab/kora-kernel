#!/bin/bash

require () {
    apt-get install -y binutils nasm xorriso
}

dbg () {
    kvm # Use serial, gdb localhost:1234, no screen
}

# -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
x86 () {
    export SRC=.
    export iso_name=KoraOs.iso
    # export target=x86-pc-smkos
    # export VERBOSE=1
    export NODEPS=1


    # make -f ../skc/Makefile
    # make -f ../skc/Makefile crt0
    # make -f ../Makefile
    make -f $SRC/Makefile kImg

    rm -rf iso

    # Import kernel and shell commands
    mkdir -p iso/{bin,boot,lib}
    cp $SRC/bin/kImg iso/boot/kImg
    # cp iso/kImg.map iso/boot/kImg.map
    # cp ../lib/* iso/lib/*
    cp ../system/hello iso/bin/init
    cp ../system/t2 iso/bin/t2
    cp ../system/t3 iso/bin/t3

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

    grub-mkrescue -o "$iso_name" iso >/dev/null
    ls -lh "$iso_name"
}


# -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
export target=x86-pc-linux-gnu
$1
