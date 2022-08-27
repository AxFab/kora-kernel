#!/usr/bin/env cli_fs
# ---------------------------------------------------------------------------

ERROR ON

TAR dsk.tar dsk
CHROOT /mnt/dsk
MOUNT - devfs /dev
INCLUDE fs_persist_rd2.sh
