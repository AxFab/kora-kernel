#!/bin/bash
set -e

SCRIPT_DIR=`dirname $BASH_SOURCE{0}`
SCRIPT_HOME=`readlink -f $SCRIPT_DIR/..`

# Select targeted architecture
CODE=$1
if [[ -z $CODE ]]
then
    CODE=`uname -m`
fi

case $CODE in
    i386|i486|i686|x86)
        export ARCH=x86
        export VENDOR=pc
        export CROSS='i686-elf-'
        export EXT=iso
        ;;
    x86_64|x64)
        export ARCH=x86_64
        export VENDOR=pc
        export CROSS='x86_64-elf-'
        export EXT=iso
        ;;
    rp1|raspberrypi1|raspberry-pi1)
        export ARCH=arm
        export VENDOR=raspberry-pi
        export CROSS='i686-elf-'
        export EXT=iso
        ;;
    rp2|raspberrypi2|raspberry-pi2)
        export ARCH=arm
        export VENDOR=raspberry-pi
        export CROSS='arm-none-eabi-'
        export EXT=iso
        ;;
    rp3|raspberrypi3|raspberry-pi3)
        export ARCH=aarch64
        export VENDOR=raspberry-pi
        export CROSS='aarch64-none-eabi-'
        export EXT=iso
        ;;
esac

# Build some variables
export VERSION='0.0.1'
export target=$ARCH-$VENDOR-kora
export SRCDIR=$SCRIPT_HOME
export GENDIR=`pwd`
export REPO='https://gitlab.com/axfab'

git_rev() {
    SHA=`git -C $SRCDIR/$1 rev-parse HEAD 2> /dev/null`
    if [[ -z $SHA ]]
    then
        echo ''
        git clone $REPO/$2 $SRCDIR/$1 1>&2
        SHA=`git -C $SRCDIR/$1 rev-parse HEAD 2> /dev/null`
    fi
    echo $SHA
}

wildcard() {
    for F in `find $2 -name $3`
    do
        echo "    $1 $F"
    done
}

kmodule() {
}

package() {
}


echo '--------------------------'
echo "Build KoraOS v$VERSION"
echo '--------------------------'
echo "Architecture: $ARCH"
echo "Vendor: $VENDOR"

GSHA_KERN=`git_rev kernel kora-kernel`

echo ''
echo "Kernel: $GSHA_KERN"
echo "LibC: $GSHA_LIBC"
echo "Drivers ($VENDOR): $GSHA_DRVS"
echo "File Systems: $GSHA_FSYS"

echo ''
echo 'Configure ------------'
# We synchronize libc and kernels headers
# LIBC contains kernel struct and info, limits.... !
# Kernel will need kora headers / and libk
# drivers will needs kernel headers and symbols
# usr app/lib, will required stdlibc or libc in case kora extention are needed.


echo ''
echo 'Build static libk for kernel ------------'

echo ''
echo 'Build kernel image ------------'

echo ''
echo 'Build drivers ------------'
kmodule drivers/$VENDOR ata

echo ''
echo 'Build file systems ------------'
kmodule fsystem ext2
kmodule fsystem fat
kmodule fsystem isofs

echo ''
echo 'Build library libc ------------'

echo ''
echo 'Configure compiler runtime ------------'

echo ''
echo 'Package 3rd party libraries ------------'
package openlibm
package zlib
package libpng
package libx
package libsh
package libgum
package krish
package browser

echo ''
echo 'Compile image ------------'

echo "    => KoraOS-$ARCH-$VENDOR-$VERSION.$EXT (1.37 Mb)"




