#!/usr/bin/env cli_fs
# ---------------------------------------------------------------------------

IMG_RM fpy.img
ERROR ON

IMG_CREATE fpy.img 1440K
IMG_OPEN fpy.img fpy
FORMAT fat12 /fpy
MOUNT /fpy fat12 /mnt/lfp '-o fat12'
CHROOT /mnt/lfp
MOUNT - devfs /dev

# ---------------------------------------------------------------------------

INCLUDE fs_mkdir.sh
INCLUDE fs_rmdir.sh
INCLUDE fs_open.sh
INCLUDE fs_unlink.sh
# INCLUDE fs_symlink.sh
# INCLUDE fs_link.sh

