/*
 *      This file is part of the SmokeOS project.
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
 *
 *      Driver for ATA API.
 */
#include <kernel/core.h>
#include <kernel/vfs.h>
#include <kernel/cpu.h>
#include <kernel/sys/inode.h>
#include <skc/mcrs.h>
#include <skc/splock.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

#ifdef K_MODULE
#  define ATA_setup setup
#  define ATA_teardown teardown
#endif

struct IMG_Drive {
  FILE *fp;
  bool ro;
  splock_t lock;
};

struct IMG_Drive sdx[4];


const char *sdNames[] = { "sdA", "sdB", "sdC", "sdD" };

const char *exts[] = {
  "img", "iso",
};
const int sdSize[] = {
  512, 2048,
};

void IMG_mkdev_img(const char *name, size_t size)
{
  FILE *fp = fopen(name, "w+");
  fseek(fp, size -1, SEEK_SET);
  fwrite(fp, 1, 1, fp);
  fclose(fp);
}

extern vfs_io_ops_t IMG_device_operations;

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

int IMG_read(inode_t *ino, void* data, size_t size, off_t offset)
{
  FILE *fp = sdx[ino->lba].fp;
  if (fp == NULL) {
    errno = ENODEV;
    return -1;
  }
  splock_lock(&sdx[ino->lba].lock);
  fseek(fp, offset, SEEK_SET);
  fread(data, size, 1, fp);
  splock_unlock(&sdx[ino->lba].lock);
  return 0;
}

int IMG_write(inode_t *ino, const void* data, size_t size, off_t offset)
{
  FILE *fp = sdx[ino->lba].fp;
  if (fp == NULL) {
    errno = ENODEV;
    return -1;
  } else if (sdx[ino->lba].ro) {
    errno = EROFS;
    return -1;
  }
  splock_lock(&sdx[ino->lba].lock);
  fseek(fp, offset, SEEK_SET);
  fwrite(data, size, 1, fp);
  splock_unlock(&sdx[ino->lba].lock);
  return 0;
}


/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

int IMG_setup()
{

  // create_empty("sdA.img", 4 * _Mib_);
  // create_empty("sdC.iso", 2800* _Kib_);

  int i, e;
  for (i = 0; i < 4; ++i) {
    char fname[16];
    for (e = 0; e < 2; ++e) {
      sprintf(fname, "sd%c.%s", 'A' + i, exts[e]);
      FILE *fp = fopen(fname, "r");
      if (fp == NULL) {
        continue;
      }

      fseek(fp, 0, SEEK_END);
      size_t sz = ftell(fp);
      kprintf(0, "%s] %s <%s> %s\n", sdNames[i], exts[e], sztoa(sz), "--");

      fclose(fp);
      // fp = fopen("sdA.img", "w+");

      inode_t *blk = vfs_inode(S_IFBLK | 0640, NULL);
      blk->length = sz;
      blk->lba = i;
      blk->block = sdSize[e];
      blk->io_ops = &IMG_device_operations;

      vfs_mkdev(sdNames[i], blk, NULL, exts[e], NULL, NULL);
      vfs_close(blk);

      // vfs_automount(blk); -- Try all or follow mount table

      // sdA  -> GPT
      // sdA1 -> Fat32 (/usr)
      // sdA2 -> Fat32 (/home)
      // sdA3 -> Ext4 (/var)
      // sdC  -> Iso9660
      break;
    }
  }

  return 0;
}


int IMG_teardown()
{
  int i;
  for (i = 0; i < 4; ++i) {
    // if (sdx[i].type_ == IDE_ATA || sdx[i].type_ == IDE_ATAPI) {
      // TODO
    // }
  }
  return 0;
}


vfs_io_ops_t IMG_device_operations = {
  .read = IMG_read,
  .write = IMG_write,
};
