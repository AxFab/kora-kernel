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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "cli-vfs.h"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Memory stub

void page_release_kmap_stub(page_t page);
void page_release(page_t page)
{
	page_release_kmap_stub(page);
}

page_t mmu_read_kmap_stub(size_t address);
page_t mmu_read(size_t vaddr)
{
	return mmu_read_kmap_stub(vaddr);
}


// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
#if 0
struct user
{
	uint8_t uuid[16];
	int uid, gid;
};

user_t *__usr_system = NULL;
user_t *usr_system()
{
	if (__usr_system == NULL) {
		__usr_system = kalloc(sizeof(user_t));
		for (int i = 0; i < 16; ++i)
			__usr_system->uuid[i] = rand8();
		__usr_system->uid = 0;
		__usr_system->gid = 0;
	}
	return __usr_system;
}

void usr_sweep()
{
	kfree(__usr_system);
}
#endif

vfs_ctx_t ctx;

void initialize()
{
	ctx.user = NULL; // usr_system();
	ctx.vfs = vfs_init();

#ifdef _EMBEDED_FS
	fat_setup();
	isofs_setup();
	ext2_setup();
#endif
}

int teardown()
{
#ifdef _EMBEDED_FS
	fat_teardown();
	isofs_teardown();
	ext2_teardown();
#endif
	vfs_sweep(ctx.vfs);
	// usr_sweep();

	return alloc_check();
}

int do_restart(vfs_t *fs, size_t *param)
{
	int ret = teardown();
	initialize();
	return ret;
}


cli_cmd_t __commands[] = {
	{ "RESTART", "", { 0, 0, 0, 0, 0 }, (void *)do_restart, 1 },
	{ "DUMP", "", { 0, 0, 0, 0, 0 }, (void *)do_dump, 1 },
	{ "UMASK", "", { ARG_INT, 0, 0, 0, 0 }, (void *)do_umask, 1 },
	{ "LS", "", { ARG_STR, 0, 0, 0, 0 }, (void *)do_ls, 1 },
	{ "STAT", "", { ARG_STR, ARG_STR, ARG_INT, ARG_STR, ARG_STR }, (void *)do_stat, 1 },
	{ "LSTAT", "", { ARG_STR, ARG_STR, ARG_INT, ARG_STR, ARG_STR }, (void *)do_lstat, 1 },
	{ "LINKS", "", { ARG_STR, ARG_INT, 0, 0, 0 }, (void *)do_links, 1 },
	{ "TIMES", "", { ARG_STR, ARG_STR, ARG_STR, 0, 0 }, (void *)do_times, 2 },
	{ "CD", "", { ARG_STR, 0, 0, 0, 0 }, (void *)do_cd, 1 },
	{ "CHROOT", "", { ARG_STR, 0, 0, 0, 0 }, (void *)do_chroot, 1 },
	{ "PWD", "", { 0, 0, 0, 0, 0 }, (void *)do_pwd, 0 },

	{ "OPEN", "", { ARG_STR, ARG_STR, ARG_INT, ARG_STR, 0 }, (void *)do_open, 2 },
	{ "CLOSE", "", { ARG_STR, 0, 0, 0, 0 }, (void *)do_close, 1 },
	{ "READ", "", { ARG_STR, ARG_STR, ARG_STR, ARG_STR, 0 }, (void *)do_read, 3 },
	{ "WRITE", "", { ARG_STR, ARG_STR, ARG_STR, ARG_STR, 0 }, (void *)do_write, 3 },
	{ "RMBUF", "", { ARG_STR, 0, 0, 0, 0 }, (void *)do_rmbuf, 1 },

	{ "DELAY", "", { ARG_STR, 0, 0, 0, 0 }, (void *)do_delay, 0 },

	{ "CREATE", "", { ARG_STR, ARG_INT, 0, 0, 0 }, (void *)do_create, 1 },
	{ "UNLINK", "", { ARG_STR, 0, 0, 0, 0 }, (void *)do_unlink, 1 },
	{ "SYMLINK", "", { ARG_STR, ARG_STR, 0, 0, 0 }, (void *)do_symlink, 2 },
	{ "LINK", "", { ARG_STR, ARG_STR, 0, 0, 0 }, (void *)do_link, 2 },

	// LINK, RENAME, SYMLINK, READLINK ...
	{ "MKDIR", "", { ARG_STR, ARG_INT, 0, 0, 0 }, (void *)do_mkdir, 1 },
	{ "RMDIR", "", { ARG_STR, 0, 0, 0, 0 }, (void *)do_rmdir, 1 },

	{ "DD", "", { ARG_STR, ARG_STR, ARG_STR, ARG_STR, 0 }, (void *)do_dd, 2 },
	{ "TRUNCATE", "", { ARG_STR, ARG_INT, 0, 0, 0 }, (void *)do_truncate, 2 },
	{ "SIZE", "", { ARG_STR, ARG_INT, 0, 0, 0 }, (void *)do_size, 1 },
	{ "CLEAR_CACHE", "", { ARG_STR, 0, 0, 0, 0 }, (void *)do_clear_cache, 1 },


	{ "MOUNT", "", { ARG_STR, ARG_STR, ARG_STR, 0, 0 }, (void *)do_mount, 3 },
	{ "UMOUNT", "", { ARG_STR, 0, 0, 0, 0 }, (void *)do_umount, 1 },

	// OPEN DIR, READ DIR, CLOSE DIR ...
	// ACCESS ...
	// FDISK !?
	// IOCTL <ino> <cmd> [..?]

	{ "EXTRACT", "", { ARG_STR, ARG_STR, 0, 0, 0 }, (void *)do_extract, 1 },
	{ "CKSUM", "", { ARG_STR, ARG_STR, 0, 0, 0 }, (void *)do_checksum, 1},
	{ "CRC32", "", { ARG_STR, ARG_STR, 0, 0, 0 }, (void *)do_checksum, 1},

	{ "IMG_OPEN", "", { ARG_STR, ARG_STR, ARG_STR, 0, 0 }, (void *)do_img_open, 2 },
	{ "IMG_CREATE", "", { ARG_STR, ARG_STR, ARG_STR, 0, 0 }, (void *)do_img_create, 2 },
	{ "IMG_COPY", "", { ARG_STR, ARG_STR, ARG_INT, 0, 0 }, (void *)do_img_copy, 2 },
	{ "IMG_RM", "",  { ARG_STR, 0, 0, 0, 0 }, (void *)do_img_remove, 1 },

	{ "TAR", "", { ARG_STR, ARG_STR, 0, 0, 0 }, (void *)do_tar_mount, 2 },

	{ "FORMAT", "", { ARG_STR, ARG_STR, ARG_STR, 0, 0 }, (void *)do_format, 2 },

	{ "CHMOD", "", { ARG_STR, ARG_STR, 0, 0, 0 }, (void *)do_chmod, 2 },
	{ "CHOWN", "", { ARG_STR, ARG_STR, 0, 0, 0 }, (void *)do_chown, 2 },
	{ "UTIMES", "", { ARG_STR, ARG_STR, ARG_STR, 0, 0 }, (void *)do_utimes, 3 },

	{ "INO", "", { ARG_STR, ARG_STR, ARG_STR, 0, 0 }, (void *)do_ino, 2 },
	{ "RENAME", "", { ARG_STR, ARG_STR, 0, 0, 0 }, (void *)do_rename, 2 },

	{ "PIPE", "", { ARG_STR, ARG_STR, 0, 0, 0 }, (void *)do_pipe, 2 },

	CLI_END_OF_COMMANDS,
};

int main(int argc, char **argv)
{
	return cli_main(argc, argv, &ctx, initialize, (void*)teardown);
}
