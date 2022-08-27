#!/usr/bin/env cli_fs
# ---------------------------------------------------------------------------

ERROR ON

IMG_OPEN dsk.iso cdrom
MOUNT /cdrom iso /mnt/ldk
CHROOT /mnt/ldk
MOUNT - devfs /dev
INCLUDE fs_persist_rd.sh
