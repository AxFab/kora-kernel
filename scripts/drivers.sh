#!/bin/bash
#      This file is part of the KoraOS project.
#  Copyright (C) 2015-2018  <Fabien Bavent>
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

DIR="$SCRIPT_HOME/src/drivers"
TMP="`readlink -f .`/drivers"
PFX="`readlink -f .`/iso/boot"

MAKE () {
    make  -f "$DIR/Makefile" bindir="$TMP/bin" srcdir="$DIR" driver="$1"
}

MAKE 'disk/ata'
MAKE 'input/ps2'
MAKE 'net/e1000'
MAKE 'media/vga'

# MAKE 'fs/ext2'
MAKE 'fs/fat'
MAKE 'fs/isofs'

cd "$TMP"
mkdir -p "$PFX"
echo "    TAR $PFX/x86.miniboot.tar"
tar cf "$PFX/x86.miniboot.tar" bin

cd "$SCRIPT_HOME"
rm -rf "$TMP"
