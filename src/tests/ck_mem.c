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
#include <kernel/core.h>
#include <kora/mcrs.h>
#include <kernel/memory.h>

inode_t *vfs_inode() {}
void vfs_open(inode_t *ino) {}
void vfs_close(inode_t *ino) {}
void vfs_read(inode_t *ino, char *buf, size_t len, off_t off) {}


void test_01()
{
    kprintf (-1, "\n\e[94m  MEM #1 - <<<>>>\e[0m\n");

    mspace_t *mem = mspace_create();
    mspace_map(mem, 0, 4 * _Kib_, NULL, 0, 0, VMA_ANON);

    mspace_rcu(mem, -1);
}


int main ()
{
    kprintf(-1, "Kora MEM check - " __ARCH " - v" _VTAG_ "\n");

    test_01();
    return 0;
}

