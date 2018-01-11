#!/bin/bash

require () {
  apt-get install -y binutils nasm xorriso
}

dbg () {
  kvm # Use serial, gdb localhost:1234, no screen
}

# -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
x86 () {
  export iso_name=KoraOs.iso
  # export target=x86-pc-smkos
  # export VERBOSE=1
  export NODEPS=1


  # make -f ../skc/Makefile
  # make -f ../skc/Makefile crt0
  # make -f ../Makefile
  make -f Makefile kImg

  rm -rf iso
  mkdir -p iso/boot/grub
  mv bin/kImg iso/boot/kImg
  mv iso/kImg.map iso/boot/kImg.map
  rm -rf iso/obj
  cat >  iso/boot/grub/grub.cfg << EOF
set default="0"
set timeout="0"

menuentry "Kora x86" {
  multiboot /boot/kImg
}
EOF

  grub-mkrescue -o "$iso_name" iso >/dev/null
  ls -l "$iso_name"
}


# -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
export target=x86-pc-linux-gnu
$1
