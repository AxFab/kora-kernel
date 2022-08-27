#!/usr/bin/env cli_fs
# ---------------------------------------------------------------------------

# Create folders and read path

# Create symlink, read symlink

# Search path with '.' and '..'

# Test readir (count elements)

# Test open (create / exclusif / open-only...)


# ---------------------------------------------------------------------------

# #
# LS /dev/Foo


# PUSHVFS open
# CHDIR /dev/zero

# POPVFS

# PUSHVFS close
# POPVFS


# IMG_CLOSE hdd.img # vfs_rmdev


# FORMAT glufs /dev/hdd # Unknown FS


# STAT Kah/Sto/Blaz
# STAT Kah/Zig/../Sto/./Blaz
# STAT Kah/Zig/../Sto/Blaz
# STAT Kah/Sto/./Blaz

# SYMLINK Kah Sto
# READLINK Sto Kah


# CHDIR Foo/Bar
# PWD L
# PWD P


# PIPE @f4
# WRITE @f4 'Hello'
# READ @f4 4 'Hell'
# # RESIZE !?
# # Several consummer (AIO) !?
# # Several producer !?

# CREATE Zig
# OPEN Zig @f5
# WRITE @f5 'Hello World'



# SCAVENGE

# IOCTL

# MKDEV (>32 entry)
# LS /dev/disk




# WRITE /dev/null
# READ /dev/null NOBLOCK # EWOULDBLOCK

# # Entropy !?


# OPEN Kah/Zig @f4
# UNLINK Zig
# RMDIR Kah
# CLOSE @f4

# CHDIR Kah/Zig
# RMDIR Zig
# RMDIR Kah
# CHDIR ..


# RMDIR /mnt/ldk # EBUZY


# OPEN **


# MOUNT ..
# UMOUNT ..



# CHOWN







