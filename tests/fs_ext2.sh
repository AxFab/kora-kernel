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
LS .
INCLUDE fs_mkdir.sh
INCLUDE fs_rmdir.sh
INCLUDE fs_open.sh
INCLUDE fs_unlink.sh
INCLUDE fs_symlink.sh
LS .
INCLUDE fs_link.sh
INCLUDE fs_chmod.sh


# ---------------------------------------------------------------------------
# ---------------------------------------------------------------------------
# Kah_Zig_Sto_Blaz_Dru_Goz_Lrz_Poo_Tbz_Gnee_Bnz_Glap

# Kah Zig Sto Blaz Dru Goz Lrz Poo Tbz Gnee Bnz Glap


# READLINK
# LINK
# CHMOD
# CHOWN
# TRUNCATE / FTRUNCATE / POSIX_FALLOCATE
# RENAME
# UTIMESAT
# IO (Check partitionned files, large files integrity...)
# MKFIFO
# MKNOD
# Check system persistance... Create a large file system with a lots of file and remount, then check them all !!
# ROFS !? (EROFS)
# MOUNT / UMOUNT / RM-FS?
