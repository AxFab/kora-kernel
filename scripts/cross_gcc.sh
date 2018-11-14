#!/bin/bash
#      This file is part of the KoraOS project.
#  Copyright (C) 2018  <Fabien Bavent>
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

export PREFIX="$SCRIPT_HOME/opt"
export SOURCES="$PREFIX/src"
export PATH="$PREFIX/bin:$PATH"

# Versions of packages
export BUTILS='binutils-2.30'
export GCC='gcc-7.3.0'


download()
{
    cd "$SOURCES"
    wget "https://ftp.gnu.org/gnu/binutils/$BUTILS.tar.xz"
    wget "https://ftp.gnu.org/gnu/gcc/$GCC/$GCC.tar.xz"

    tar xvf "$BUTILS.tar.xz"
    tar xvf "$GCC.tar.xz"

    cd "$GCC"
    ./contrib/download_prerequisites
}


build_binutils()
{
    cd "$SOURCES"

    mkdir build-binutils
    cd build-binutils
    "../$BUTILS/configure" --target=$TARGET --prefix="$PREFIX" --with-sysroot --disable-nls --disable-werror
    make
    make install
}

build_gcc()
{
    cd "$SOURCES"
    # The $PREFIX/bin dir _must_ be in the PATH. We did that above.
    which -- $TARGET-as || echo $TARGET-as is not in the PATH

    mkdir build-gcc
    cd build-gcc
    "../$GCC/configure" --target=$TARGET --prefix="$PREFIX" --disable-nls --enable-languages=c,c++ --without-headers
    make all-gcc
    make all-target-libgcc
    make install-gcc
    make install-target-libgcc
}

clean () {
    rm -rf "$SOURCES"
}

look_arch() {
    case "$1"
    in
        i386|i486|i686) export TARGET='i386-elf' ;;
        raspi2) export TARGET='arm-none-eabi' ;;
    esac
}

mkdir -p $PREFIX/{bin,lib,src}

download
build_binutils
build_gcc

# echo '#!/bin/sh' > "$SCRIPT_HOME/.toolchain"
# echo 'export PATH="'$PREFIX'/bin":$PATH' >> "$SCRIPT_HOME/.toolchain"
# echo 'export CROSS="'$TARGET'-"' >> "$SCRIPT_HOME/.toolchain"
