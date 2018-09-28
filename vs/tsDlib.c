/*
*      This file is part of the KoraOS project.
*  Copyright (C) 2015  <Fabien Bavent>
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
#include <string.h>
#include <stdio.h>
#include "dlib.h"
#include "elf.h"

#if !defined(_WIN32)
#define _valloc(s) valloc(s)
#define _vfree(p) free(p)
#else
#define _valloc(s) _aligned_malloc(s, PAGE_SIZE)
#define _vfree(p) _aligned_free(p)
#endif

int vfs_read() { return -1; }
int vfs_write() { return -1; }

void *bio_access(void* ptr, int lba)
{
    return ADDR_OFF(ptr, lba * PAGE_SIZE);
}

void bio_clean(void* ptr, int lba)
{
}

bio_t *bio_setup(CSTR name)
{
    FILE *fp;
    fopen_s(&fp, name, "rb");
    fseek(fp, 0, SEEK_END);
    int lg = ftell(fp);
    uint8_t *ptr = _valloc(ALIGN_UP(lg, PAGE_SIZE));
    memset(ptr, 0, ALIGN_UP(lg, PAGE_SIZE));
    fseek(fp, 0, SEEK_SET);
    fread(ptr, 1, lg, fp);
    return (bio_t*)ptr;
}

int main()
{
    dynlib_t dlib;
    memset(&dlib, 0, sizeof(dlib));
    dlib.name = strdup("imgdk.km");
    dlib.rcu = 1;
    dlib.io = bio_setup("imgdk.so");
    int ret = elf_parse(&dlib);
    

    return 0;
}
