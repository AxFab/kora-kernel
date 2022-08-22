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
#include "../cli.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <kora/mcrs.h>
#include <kernel/stdc.h>

void vfs_write() {}
void vfs_search() {}
void vfs_usage() {}
void vfs_inodeof() {}
void vfs_open_fnode() {}
void vfs_close_fnode() {}
void vfs_open_inode() {}
void vfs_close_inode() {}
void vfs_open_vfs() {}
void vfs_clone_vfs() {}

void mspace_open() {}
void mspace_create() {}
void mspace_map() {}
void mspace_unmap() {}
void mspace_protect() {}

void mmu_context() {}

void irq_zero() {}

void cpu_halt() {}
void cpu_save() {}
void cpu_restore() {}
void cpu_prepare() {}
void cpu_usermode() {}


// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

void initialize()
{
}

void teardown()
{
}

void do_restart(void *ctx, size_t *param)
{
    teardown();
    // TODO -- Check all is clean
    initialize();
}


cli_cmd_t __commands[] = {
    { "RESTART", "", { 0, 0, 0, 0, 0 }, (void*)do_restart, 1 },

    CLI_END_OF_COMMANDS,
};

int main(int argc, char **argv)
{
    return cli_main(argc, argv, NULL, initialize, teardown);
}
