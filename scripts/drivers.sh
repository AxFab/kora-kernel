#!/bin/bash
#      This file is part of the KoraOS project.
#  Copyright (C) 2015-2021  <Fabien Bavent>
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

function parse_yaml {
    local prefix=$2
    local s='[[:space:]]*' w='[a-zA-Z0-9_]*' fs=$(echo @|tr @ '\034')
    sed -ne "s|^\($s\):|\1|" \
        -e "s|^\($s\)\($w\)$s:$s[\"']\(.*\)[\"']$s\$|\1$fs\2$fs\3|p" \
        -e "s|^\($s\)\($w\)$s:$s\(.*\)$s\$|\1$fs\2$fs\3|p"  $1 |
    awk -F$fs '{
        indent = length($1)/2;
        vname[indent] = $2;
        for (i in vname) {
            if (i > indent) {delete vname[i]}}
            if (length($3) > 0) {
                vn="";
                for (i=0; i<indent; i++) {vn=(vn)(vname[i])("_")}
            printf("%s%s%s=\"%s\"\n", "'$prefix'",vn, $2, $3);
            }
        }'
}


function make_driver {
    make  -f "$DIR/Makefile" bindir="$TMP/bin" srcdir="$DIR" driver="$1"
}


. <(parse_yaml $SCRIPT_HOME/config.yml)
for d in `find $DIR/ -type d | sed "s%$DIR/%%"`
do
    if [[ -z `echo $d | grep .git` ]]
    then
        v=drivers_`echo $d | sed s%/%_%g`
        if [[ ${!v} == 'y' ]]
        then
            make_driver $d
        fi
    fi
done


cd "$TMP"
mkdir -p "$PFX"
echo "    TAR $PFX/x86.miniboot.tar"
tar cf "$PFX/x86.miniboot.tar" bin

cd "$SCRIPT_HOME"
rm -rf "$TMP"
