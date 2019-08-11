#include <bits/cdefs.h>
#include <stdatomic.h>
#include <kernel/types.h>
#include <kernel/core.h>
#include "check.h"

// #include <stdio.h>
// #include <stdlib.h>


void sleep_timer(long timeout)
{
    usleep(MAX(1, timeout));
}


