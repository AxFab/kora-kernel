/*
 *      This file is part of the KoraOS project.
 *  Copyright (C) 2015-2021  <Fabien Bavent>
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
#ifndef SYS_SEM_H
#define SYS_SEM_H 1

#include <threads.h>

typedef struct sem sem_t;

struct sem {
    mtx_t mtx;
    cnd_t cv;
    int count;
};

int sem_init(sem_t *sem, int count);
void sem_destroy(sem_t *sem);
void sem_acquire(sem_t *sem);
int sem_timedacquire(sem_t *sem, const struct timespec *restrict time_point);
void sem_acquire_many(sem_t *sem, int count);
int sem_tryacquire(sem_t *sem);
void sem_release(sem_t *sem);
void sem_release_many(sem_t *sem, int count);


#endif /* SYS_SEM_H */
