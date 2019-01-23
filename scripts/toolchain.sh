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


PACK='kora-toolchain-x86_64.tar.bz2'
URL="https://www.dropbox.com/s/wzjsiqbabyj1t7g/$PACK"

install () {
    cd $SCRIPT_HOME
    wget $URL -o wget.log
    tar xjf $PACK
}


while (( $# > 0 ))
do
    case $1
    in
        install)
            $1
        ;;
    esac
    shift
done

