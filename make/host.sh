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
#  This makefile is more or less generic.
#  The configuration is on `sources.mk`.
# -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

# Parse first argument if provided
if [ -n $1 ]; then
    IFS='-' read -ra THST <<< "$1"
    arch=${THST[0]}
    vendor=${THST[1]}
    req_vendor=${THST[1]}
    os=`tr ' ' '-' <<< "${THST[@]:2:${#THST[@]}}" `
fi

# Find architecture
if [ -z "$arch" ]; then
    arch=`uname -m`
fi

case "$arch" in
    i[3-7]86)
        arch=i386
        vendor=pc
        ;;
    x86_64|amd64)
        arch=amd64
        vendor=pc
        ;;
    armv7l)
        arch=arm
        vendor=phone
        ;;
    *)
        echo "Unsupported architecture" >&2
        exit 1
        ;;
esac

# Check vendor
if [ -n $1 ]; then
    if [ "$vendor" != "$req_vendor" ] ; then
          os=`tr ' ' '-' <<< "${THST[@]:1:${#THST[@]}}" `
    fi
fi

# Resolve operating system
if [ -z "$os" ]; then
    os=`uname -o`
fi

case "$os" in
    Android)
        os=linux-android
        ;;
    GNU/Linux)
        os=linux-gnu
        ;;
    kora)
        os=kora
        ;;
    *)
        echo "Unsupported platform " >&2
        exit 1
        ;;
esac

# Print final triplet result
echo "$arch-$vendor-$os"
