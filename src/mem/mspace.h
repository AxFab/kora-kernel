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
#ifndef _SRC_MSPACE_H
#define _SRC_MSPACE_H 1

#include <kernel/core.h>
#include <kernel/memory.h>
#include <kora/bbtree.h>
#include <kora/splock.h>
#include <bits/atomic.h>


struct vma {
    bbnode_t node;  /* Binary tree node of VMAs contains the base address */
    size_t length;  /* The length of this VMA */
    inode_t *ino;  /* If the VMA is linked to a file, a pointer to the inode */
    off_t offset;  /* An offset to a file or an physical address, depending of type */
    int flags;  /* VMA flags */
    mspace_t *mspace;
};


/* Helper to print VMA info into syslogs */
char *vma_print(char *buf, int len, vma_t *vma);
/* - */
vma_t *vma_create(mspace_t *mspace, size_t address, size_t length, inode_t *ino, off_t offset, int flags);
/* - */
vma_t *vma_clone(mspace_t *mspace, vma_t *model);
/* Split one VMA into two. */
vma_t *vma_split(mspace_t *mspace, vma_t *area, size_t length);
/* Close a VMA and release private pages. */
int vma_close(mspace_t *mspace, vma_t *vma, int arg);
/* Change the flags of a VMA. */
int vma_protect(mspace_t *mspace, vma_t *vma, int flags);


int vma_resolve(vma_t *vma, size_t address, size_t length);

int vma_copy_on_write(vma_t *vma, size_t address, size_t length);



/* Close all VMAs */
void mspace_sweep(mspace_t *mspace);


#endif  /* _SRC_MSPACE_H */
