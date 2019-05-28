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
#ifndef _KORA_FUTEX_H
#define _KORA_FUTEX_H 1

#include <threads.h>
#include <time.h>
#include <kora/splock.h>

#define FUTEX_PRIORITY  1
#define FUTEX_REALTIME  2
#define FUTEX_SHARED  4
#define FUTEX_CREATE  8

typedef long long tick_t;

int futex_wait(int *addr, int val, long timeout, int flags);
int futex_wake(int *addr, int val);
int futex_requeue(int *addr, int val, int val2, int *addr2, int flags);
tick_t futex_tick();
void futex_init();

tick_t clock_read(int no);

#endif /* _KORA_FUTEX_H */
