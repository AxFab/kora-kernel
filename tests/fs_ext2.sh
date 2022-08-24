#!/usr/bin/env cli_fs
# ---------------------------------------------------------------------------

IMG_RM hdd.img
ERROR ON

IMG_CREATE hdd.img 24M
IMG_OPEN hdd.img hdd
FORMAT ext2 /hdd
MOUNT /hdd ext2 /mnt/ldk
CHROOT /mnt/ldk
MOUNT - devfs /dev

# ---------------------------------------------------------------------------

LS .
INCLUDE fs_mkdir.sh
INCLUDE fs_rmdir.sh
INCLUDE fs_open.sh
INCLUDE fs_unlink.sh
INCLUDE fs_symlink.sh
INCLUDE fs_link.sh
INCLUDE fs_chmod.sh
# INCLUDE fs_chown.sh
INCLUDE fs_truncate.sh
INCLUDE fs_rename.sh
INCLUDE fs_utimes.sh
INCLUDE fs_readwrite.sh
INCLUDE fs_paths.sh

# INCLUDE fs_persist1.sh # Create large files hierarchy
# CHROOT /dev
# UMOUNT /mnt/ldk/dev
# UMOUNT /mnt/ldk
# RESTART
# IMG_OPEN hdd.img hdd
# MOUNT /hdd ext2 /mnt/ldk
# CHROOT /mnt/ldk
# MOUNT - devfs /dev
# INCLUDE fs_persist2.sh # Read large files hierarchy
# INCLUDE fs_rofs.sh

# ---------------------------------------------------------------------------
# ---------------------------------------------------------------------------
# Kah_Zig_Sto_Blaz_Dru_Goz_Lrz_Poo_Tbz_Gnee_Bnz_Glap
# Kah Zig Sto Blaz Dru Goz Lrz Poo Tbz Gnee Bnz Glap
