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
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <bits/cdefs.h>
#include <threads.h>
#if defined(_WIN32)
#  include <Windows.h>
#endif



struct task_data_start {
    char name[64];
    void (*func)(void*);
    void* arg;
};

static int _task_impl_start(void* arg)
{
    struct task_data_start* data = arg;
#ifdef _WIN32
    wchar_t wbuf[64];
    size_t len;
    mbstowcs_s(&len, wbuf, 64, data->name, 64);
    SetThreadDescription(GetCurrentThread(), wbuf);
#endif
    void(*func)(void*) = data->func;
    void* param = data->arg;
    free(data);
    func(param);
    return 0;
}

int task_start(const char* name, void *func, void* arg)
{
    thrd_t thrd;
    struct task_data_start* data = malloc(sizeof(struct task_data_start));
    assert(data != NULL);
    strncpy(data->name, name, 64);
    data->func = func;
    data->arg = arg;
    thrd_create(&thrd, _task_impl_start, data);
    return 0;
}

