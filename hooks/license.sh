#!/bin/bash

SCRIPT_DIR=`dirname $BASH_SOURCE{0}`
SCRIPT_HOME=`readlink -f $SCRIPT_DIR/..`


LICENSE=`head $SCRIPT_DIR/license.h -n 18`
for src in `find $SCRIPT_HOME -type f -a -name '*.c' -o -name '*.h'`
do
  SRC_HEAD=`head $src -n 18`
  if [ "$LICENSE" != "$SRC_HEAD" ]
  then
    echo "Missing license: $src"
  fi
done
