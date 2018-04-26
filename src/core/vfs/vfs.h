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
#ifndef _SRC_VFS_H
#define _SRC_VFS_H 1

#include <kernel/mods/fs.h>
#include <kora/llist.h>
#include <kora/rwlock.h>

typedef struct dirent dirent_t;

struct dirent {
    inode_t *parent;
    inode_t *ino;
    llnode_t node;
    llnode_t lru;
    rwlock_t lock;
    int lg;
    char key[256 + 4];
};


dirent_t *vfs_dirent_(inode_t *dir, CSTR name, bool block);
void vfs_set_dirent_(dirent_t *ent, inode_t *ino);
void vfs_rm_dirent_(dirent_t *ent);

void vfs_record_(inode_t *dir, inode_t *ino);

dirent_t *vfs_lookup_(inode_t *dir, CSTR name);
inode_t *vfs_search_(inode_t *top, CSTR path, acl_t *acl, int *links);

void vfs_mountpt_rcu_(mountfs_t *fs);

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */


#endif  /* _SRC_VFS_H */
