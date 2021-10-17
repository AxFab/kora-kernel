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
#include <threads.h>
#include <sys/sem.h>
#include <assert.h>

int sem_init(sem_t *sem, int count)
{
    if (sem == NULL)
        return thrd_error;

    sem->count = count;
    int res = mtx_init(&sem->mtx, mtx_plain);
    if (res == thrd_success)
        res = cnd_init(&sem->cv);
    return res;
}

void sem_destroy(sem_t *sem)
{
    assert(sem != NULL);
    mtx_destroy(&sem->mtx);
    cnd_destroy(&sem->cv);
}

void sem_acquire(sem_t *sem)
{
    assert(sem != NULL);
    mtx_lock(&sem->mtx);
    while (sem->count == 0)
        cnd_wait(&sem->cv, &sem->mtx);
    --sem->count;
    mtx_unlock(&sem->mtx);
}

void sem_acquire_many(sem_t *sem, int count)
{
    assert(sem != NULL);
    mtx_lock(&sem->mtx);
    while (sem->count < count)
        cnd_wait(&sem->cv, &sem->mtx);
    sem->count -= count;
    mtx_unlock(&sem->mtx);
}

int sem_tryacquire(sem_t *sem)
{
    assert(sem != NULL);
    mtx_lock(&sem->mtx);
    if (sem->count == 0) {
        mtx_unlock(&sem->mtx);
        return thrd_busy;
    }
    --sem->count;
    mtx_unlock(&sem->mtx);
    return thrd_success;
}

void sem_release(sem_t *sem)
{
    assert(sem != NULL);
    mtx_lock(&sem->mtx);
    ++sem->count;
    cnd_signal(&sem->cv);
    mtx_unlock(&sem->mtx);
}


void sem_release_many(sem_t *sem, int count)
{
    assert(sem != NULL);
    mtx_lock(&sem->mtx);
    sem->count += count;
    cnd_signal(&sem->cv);
    mtx_unlock(&sem->mtx);
}

int sem_drain(sem_t *sem)
{
    int count;
    assert(sem != NULL);
    mtx_lock(&sem->mtx);
    count = sem->count;
    sem->count = 0;
    if (count < 0)
        cnd_signal(&sem->cv);
    mtx_unlock(&sem->mtx);
    return count;
}
