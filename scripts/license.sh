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

fixSrc_file=$SCRIPT_DIR/cfg/license.h
fixSh_file=$SCRIPT_DIR/cfg/license.txt

fixSrc() {
    TMP=`mktemp`
    cat $1 > $TMP
    cat $2 <(awk '/^#(include|ifndef)/ || c>0 {print;++c}' $TMP) > $1
    rm $TMP
}

fixSh() {
    TMP=`mktemp`
    cat $1 > $TMP
    cat $2 <(awk '/^SCRIPT_DIR/ || c>0 {print;++c}' $TMP) > $1
    rm $TMP
}


checkDir () {
    FILE=$1'_file'
    MODEL=${!FILE}
    LICENSE=`head "$MODEL" -n 18`
    for src in $2
    do
        SRC_HEAD=`head -n18 $src`
        if [ "$LICENSE" != "$SRC_HEAD" ]
        then
            if [ "$FIX" != 'y' ]
            then
                echo "Missing license: $src"
            else
                echo "Editing license: $src"
                $1 "$src" "$MODEL"
            fi
        else
             echo "License OK: $src"
        fi
    done
}

checkDir fixSrc "`find . -type f -a -name '*.c' -o -name '*.h'`"
checkDir fixSh "`find . -type f -a -name '*.sh'`"

