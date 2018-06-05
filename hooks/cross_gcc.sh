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

export PREFIX="$SCRIPT_HOME/opt"
export SOURCES="$PREFIX/src"
export TARGET=i686-elf
export PATH="$PREFIX/bin:$PATH"

# Versions of packages
export BUTILS='binutils-2.30'
export GCC='gcc-7.3.0'
export GMP='gmp-6.1.2'
export MPC='mpc-1.0.2'
export MPFR='mpfr-4.0.1'
export ISL='isl-0.16.1'

download()
{
    cd $SOURCES
}

# Extract sources
extract()
{
    cd $SOURCES

    tar xzf "$BUTILS.tar.gz"
    tar xzf "$GCC.tar.gz"
    tar xjf "$GMP.tar.bz2"
    tar xzf "$MPC.tar.gz"
    tar xzf "$MPFR.tar.gz"

    ln -s "../$GMP" "$GCC/gmp"
    ln -s "../$MPC" "$GCC/mpc"
    ln -s "../$MPFR" "$GCC/mpfr"
    ln -s "../$ISL" "$GCC/isl"
}

build_binutils()
{
    cd $SOURCES

    mkdir build-binutils
    cd build-binutils
    "../$BUTILS/configure" --target=$TARGET --prefix="$PREFIX" --with-sysroot --disable-nls --disable-werror
    make
    make install
}

build_gcc()
{
    cd $SOURCES
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



mkdir -p $PREFIX/{bin,lib,src}

download
extract
build_binutils
build_gcc

# echo '#!/bin/sh' > "$SCRIPT_HOME/.toolchain"
# echo 'export PATH="'$PREFIX'/bin":$PATH' >> "$SCRIPT_HOME/.toolchain"
# echo 'export CROSS="'$TARGET'-"' >> "$SCRIPT_HOME/.toolchain"
