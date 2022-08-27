
IMG_RM vA.vhd
IMG_RM vB.vhd
IMG_RM vC.vhd
IMG_RM vD.vhd

ERROR ON


# ----------------------------------------------------------------------------
# Pipe --
PIPE @pr @pw

READ @bf0 random 64
WRITE @bf0 @pw 64
READ @bf1 @pr 64

READ @bf0 random 2048
WRITE @bf0 @pw 2048
WRITE @bf0 @pw 2048
WRITE @bf0 @pw 2048
WRITE @bf0 @pw 2048
READ @bf1 @pr 8196

CLOSE @pw
CLOSE @pr

RMBUF @bf0
RMBUF @bf1


# ----------------------------------------------------------------------------
# Block device
IMG_CREATE vA.vhd 1M
IMG_CREATE vB.vhd 4M
IMG_CREATE vC.vhd 4M
IMG_CREATE vD.vhd 16M

LS disk/by-uuid 0
IMG_OPEN vA.vhd vA
LS disk/by-uuid 1

IMG_OPEN vB.vhd vB
IMG_OPEN vC.vhd vC
IMG_OPEN vD.vhd vD

# ----------------------------------------------------------------------------
# Mount & unmount
TAR dsk.tar dsk
CHROOT /mnt/dsk
MOUNT - devfs /dev
UMOUNT /dev



# ----------------------------------------------------------------------------
# Zombie case
# MOUNT - devfs /dev
# FORMAT ext2 /dev/vB
# MOUNT /dev/vB ext2 /dev/mnt/dkb
# CD /dev/mnt/dkb
# PWD

# MKDIR Poo

# OPEN Poo/Foo RDONLY,CREATE,EXCL @fl
# UNLINK Poo/Foo
# CLOSE @fl

# OPEN Poo/Bar RDONLY,CREATE,EXCL @fl
# UNLINK Poo/Bar
# RMDIR Poo
# CLOSE @fl


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

# INCLUDE fs_paths.sh


# # ----------------------------------------------------------------------------
# ERROR OFF

# IMG_RM vA.vhd
# IMG_RM vB.vhd
# IMG_RM vC.vhd
# IMG_RM vD.vhd
