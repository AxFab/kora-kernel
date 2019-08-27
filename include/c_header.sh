#!/bin/bash

TOPDIR=`pwd`

function header {

    DEF=`echo "${1^^}" | sed -e 's%/%_%g' -e 's%\.%_%g'`

    cat > $TOPDIR/$1 << EOF
#ifndef __$DEF
#define __$DEF 1

#include <stddef.h>


#endif  /* __$DEF */
EOF

}


while [[ $# > 0 ]]; do
    header $1
    shift
done

