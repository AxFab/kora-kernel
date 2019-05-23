/*
 *      This file is part of the KoraOS project.
 *  Copyright (C) 2015-2019  <Fabien Bavent>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Affero General Public License as
 *  published by the Free Software Foundation, either version 3 of the
 *  License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Affero General Public License for more details.
 *
 *  You should have received a copy of the GNU Affero General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *   - - - - - - - - - - - - - - -
 */
#include <threads.h>
#include <kernel/task.h>
#include <kora/splock.h>
#include <errno.h>
#include <limits.h>


struct _US_MUTEX {
    atomic_int counter;
    int flags;
    thrd_t thread;
    splock_t splock;
    emitter_t emitter;
};

thrd_t thrd_current(void)
{
    return NULL;
}

int thrd_equal(thrd_t l, thrd_t r)
{
    return l == r;
}
