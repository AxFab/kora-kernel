# Highly inspirated from `pjdfstest` testing program
# ---------------------------------------------------------------------------

# License for all regression tests available with pjdfstest:
#
# Copyright (c) 2006-2012 Pawel Jakub Dawidek <pawel@dawidek.net>
# All rights reserved.
# 
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
# 
# THIS SOFTWARE IS PROVIDED BY THE AUTHORS AND CONTRIBUTORS ``AS IS'' AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE
# FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
# OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
# HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
# LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
# OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
# SUCH DAMAGE.
# ---------------------------------------------------------------------------

# Create, format and mount an empty volume
ERROR ON

# Kah Zig Sto Blaz Dru Goz Lrz Poo Tbz Gnee Bnz Glap 
# ---------------------------------------------------------------------------
# ---------------------------------------------------------------------------
# Directory creation file permission
UMASK 022

MKDIR Kah 0755
STAT Kah DIR 0755
LINKS Kah 2

MKDIR Kah/Zig 0755
STAT Kah/Zig DIR 0755
LINKS Kah/Zig 2
LINKS Kah 3

MKDIR Kah/Sto 0151
STAT Kah/Sto DIR 0151
LINKS Kah/Sto 2
LINKS Kah 4

UMASK 077
MKDIR Kah/Blaz 0151
STAT Kah/Blaz DIR 0100
LINKS Kah/Blaz 2
LINKS Kah 5

UMASK 070
MKDIR Kah/Dru 0345
STAT Kah/Dru DIR 0305
LINKS Kah/Dru 2
LINKS Kah 6

UMASK 0501
MKDIR Kah/Goz 0345
STAT Kah/Goz DIR 0244
LINKS Kah/Goz 2
LINKS Kah 7

RMDIR Kah/Zig
LINKS Kah 6
RMDIR Kah/Sto
LINKS Kah 5
RMDIR Kah/Blaz
LINKS Kah 4
RMDIR Kah/Dru
LINKS Kah 3
RMDIR Kah/Goz
LINKS Kah 2
RMDIR Kah

# Directory creation user id
# UMASK 022
# SETEID 65535 65535
# MKDIR Lrz 0755

# MKDIR Lrz/Poo 0755
# STAT Lrz/Poo 0755 65535 65535

# SETEID 65535 65534
# MKDIR Lrz/Tbz 0755
# STAT Lrz/Tbz 0755 65535 65534/65535

# SETEID 65534 65533
# ERROR FAIL
# MKDIR Lrz/Gnee 0755
# ERROR ON

# SETEID 65535 65535
# CHMOD Lrz 0777
# SETEID 65534 65533
# MKDIR Lrz/Gnee 0755
# STAT Lrz/Gnee 0755 65534 65533/65535

# RMDIR Lrz/Poo
# RMDIR Lrz/Tbz
# RMDIR Lrz/Gnee
# RMDIR Lrz

# Directory creation file dates
UMASK 022
# SETEID 0 0

MKDIR Bnz
TIMES Bnz C now
DELAY
TIMES Bnz C lt-now

MKDIR Bnz/Glap
TIMES Bnz/Glap AMC now
TIMES Bnz MC now

RMDIR Bnz/Glap
RMDIR Bnz

# ------------------------------------------------
# Directory creation returns ENOTDIR
UMASK 022
MKDIR Kah

CREAT Kah/Zig
CLEAR_CACHE Kah/Zig
ERROR ENOTDIR
MKDIR Kah/Zig/Sto
ERROR ON
UNLINK Kah/Zig

# SYMLINK Kah/Zig a/b/c
# CLEAR_CACHE Kah/Zig
# ERROR ENOTDIR
# MKDIR Kah/Zig/Sto
# ERROR ON
# UNLINK Kah/Zig

RMDIR Kah

# ------------------------------------------------
# Directory creation returns ENAMETOOLONG
UMASK 022
MKDIR Kah

ERROR ENAMETOOLONG
MKDIR Kah/Kah_Zig_Sto_Blaz_Dru_Goz_Lrz_Poo_Tbz_Gnee_Bnz_Glap_Kah_Zig_Sto_Blaz_Dru_Goz_Lrz_Poo_Tbz_Gnee_Bnz_Glap_Kah_Zig_Sto_Blaz_Dru_Goz_Lrz_Poo_Tbz_Gnee_Bnz_Glap_Kah_Zig_Sto_Blaz_Dru_Goz_Lrz_Poo_Tbz_Gnee_Bnz_Glap_Kah_Zig_Sto_Blaz_Dru_Goz_Lrz_Poo_Tbz_Gnee_Bnz_Glap_Kah_Zig_Sto_Blaz_Dru_Goz_Lrz_Poo_Tbz_Gnee_Bnz_Glap
ERROR ON

RMDIR Kah

# ------------------------------------------------
# Directory creation returns ENOENT
UMASK 022
MKDIR Kah

ERROR ENOENT
MKDIR Kah/Zig/Sto
ERROR ON

RMDIR Kah

# ------------------------------------------------
# # Directory creation returns EACCES
# UMASK 022
# SETEID 0 0
# MKDIR Kah

# MKDIR Kah/Zig 0755
# CHOWN Kah/Zig 65534 65534

# SETEID 65534 65534

# MKDIR Kah/Zig/Sto
# RMDIR Kah/Zig/Sto

# CHMOD Kah/Zig 0644
# ERROR EACCES
# MKDIR Kah/Zig/Sto
# ERROR ON

# CHMOD Kah/Zig 0555
# ERROR EACCES
# MKDIR Kah/Zig/Sto
# ERROR ON

# CHMOD Kah/Zig 0755
# MKDIR Kah/Zig/Sto
# RMDIR Kah/Zig/Sto

# RMDIR Kah/Zig
# RMDIR Kah

# ------------------------------------------------
# # Directory creation returns ELOOP
# UMASK 022
# SETEID 0 0

# SYMLINK Kah Zig
# SYMLINK Zig Kah

# ERROR ELOOP
# MKDIR Kah/Sto
# MKDIR Zig/Sto
# ERROR ON

# UNLINK Kah
# UNLINK Zig

# ------------------------------------------------
# Directory creation returns EEXIST

MKDIR Kah

CREAT Kah/Zig
CLEAR_CACHE Kah/Zig
ERROR EEXIST
MKDIR Kah/Zig
ERROR ON
# CLI
UNLINK Kah/Zig

# SYMLINK Kah/Zig a/b/c
# CLEAR_CACHE Kah/Zig
# ERROR EEXIST
# MKDIR Kah/Zig
# ERROR ON
# UNLINK Kah/Zig

MKDIR Kah/Zig
CLEAR_CACHE Kah/Zig
ERROR EEXIST
MKDIR Kah/Zig
ERROR ON
RMDIR Kah/Zig

RMDIR Kah

# ------------------------------------------------
# Directory creation returns ENOSPC
# TODO -- Fill all inode tables until disk is full

# ---------------------------------------------------------------------------
# ---------------------------------------------------------------------------
# Directory remove
MKDIR Kah 0755
STAT Kah DIR 0755
RMDIR Kah
ERROR ENOENT
STAT Kah DIR 0755
ERROR ON

MKDIR Zig 0755
MKDIR Zig/Sto 0755
TIMES Zig/Sto C now
DELAY

RMDIR Zig/Sto 0755
TIMES Zig MC now

RMDIR Zig

# ------------------------------------------------
# Directory remove returns ENOTDIR

MKDIR Kah 0755
CREAT Kah/Zig 0644
ERROR ENOTDIR
RMDIR Kah/Zig/Sto
ERROR ON
UNLINK Kah/Zig
RMDIR Kah

CREAT Sto 0644
ERROR ENOTDIR
RMDIR Sto/Goz
ERROR ON
UNLINK Sto

# SYMLINK Blaz Sto
# ERROR ENOTDIR
# RMDIR Blaz/Goz
# ERROR ON
# UNLINK Blaz

# ------------------------------------------------
# Directory remove returns ENAMETOOLONG
MKDIR Kah 0755
RMDIR Kah

ERROR ENOENT
RMDIR Kah

ERROR ENAMETOOLONG
MKDIR Kah_Zig_Sto_Blaz_Dru_Goz_Lrz_Poo_Tbz_Gnee_Bnz_Glap_Kah_Zig_Sto_Blaz_Dru_Goz_Lrz_Poo_Tbz_Gnee_Bnz_Glap_Kah_Zig_Sto_Blaz_Dru_Goz_Lrz_Poo_Tbz_Gnee_Bnz_Glap_Kah_Zig_Sto_Blaz_Dru_Goz_Lrz_Poo_Tbz_Gnee_Bnz_Glap_Kah_Zig_Sto_Blaz_Dru_Goz_Lrz_Poo_Tbz_Gnee_Bnz_Glap_Kah_Zig_Sto_Blaz_Dru_Goz_Lrz_Poo_Tbz_Gnee_Bnz_Glap
ERROR ON

# ------------------------------------------------
# Directory remove returns ENOENT
MKDIR Kah 0755
RMDIR Kah

ERROR ENOENT
RMDIR Kah
RMDIR Zig
ERROR ON

# ------------------------------------------------
# Directory remove returns ELOOP
# SYMLINK Kah Zig
# SYMLINK Zig Kah

# ERROR ELOOP
# RMDIR Kah/Sto
# RMDIR Zig/Sto
# ERROR ON

# UNLINK Kah
# UNLINK Zig

# ------------------------------------------------
# Directory remove returns ENOTEMPTY
MKDIR Kah

CREAT Kah/Zig
ERROR ENOTEMPTY
RMDIR Kah
ERROR ON
UNLINK Kah/Zig

MKDIR Kah/Sto
ERROR ENOTEMPTY
RMDIR Kah
ERROR ON
RMDIR Kah/Sto

# SYMLINK Kah/Blaz
# ERROR ENOTEMPTY
# RMDIR Kah
# ERROR ON
# UNLINK Kah/Blaz

RMDIR Kah

# ------------------------------------------------
# Directory remove returns EACCES


# ------------------------------------------------
# Directory remove returns EPERM

# ------------------------------------------------
# Directory remove returns EINVAL
# MKDIR Kah
# MKDIR Kah/Zig
# ERROR EINVAL
# RMDIR Kah/Zig/.
# RMDIR Kah/Zig/..
# ERROR ON
# RMDIR Kah/Zig
# RMDIR Kah


# ------------------------------------------------
# Directory remove returns EBUSY
# MKDIR Kah
# # MKDIR Kah/Zig
# MOUNT - devfs Kah/Zig
# ERROR EBUSY
# RMDIR Kah/Zig
# ERROR ON
# UMOUNT Kah/Zig
# # RMDIR Kah/Zig
# RMDIR Kah


# ---------------------------------------------------------------------------
# ---------------------------------------------------------------------------
# Open and create file - Check mode
UMASK 022
MKDIR Kah

OPEN Kah/Zig CREAT:WRONLY 0755
STAT Kah/Zig REG 0755
UNLINK Kah/Zig

OPEN Kah/Sto CREAT:WRONLY 0151
STAT Kah/Sto REG 0151
UNLINK Kah/Sto

UMASK 077
OPEN Kah/Blaz CREAT:WRONLY 0151
STAT Kah/Blaz REG 0100
UNLINK Kah/Blaz

UMASK 070
OPEN Kah/Dru CREAT:WRONLY 0345
STAT Kah/Dru REG 0305
UNLINK Kah/Dru

UMASK 0501
OPEN Kah/Goz CREAT:WRONLY 0345
STAT Kah/Goz REG 0244
UNLINK Kah/Goz

UMASK 022
RMDIR Kah

# ---------------------------------------------------------------------------
# Open and create file - Check ownership
# MKDIR Kah
# CHOWN Kah 65535 65535

# SETEID 65535 65535
# OPEN Kah/Zig CREAT:WRONLY 0644
# STAT Kah/Zig REG 0644 65535 65535
# UNLINK Kah/Zig

# SETEID 65535 65534
# OPEN Kah/Sto CREAT:WRONLY 0644
# STAT Kah/Sto REG 0644 65535 65534/65535
# UNLINK Kah/Sto

# SETEID 0 0
# CHMOD Kah 0777
# SETEID 65534 65533
# OPEN Kah/Blaz CREAT:WRONLY 0644
# STAT Kah/Blaz REG 0644 65534 65533/65535
# UNLINK Kah/Blaz

# SETEID 0 0
# RMDIR Kah

# ---------------------------------------------------------------------------
# Open and create file - Check times
MKDIR Kah
TIMES Kah C now
DELAY

TIMES Kah C lt-now
OPEN Kah/Lrz CREAT:WRONLY 0644
TIMES Kah/Lrz CMA now
TIMES Kah CM now

DELAY
OPEN Kah/Lrz CREAT:RDONLY 0644
TIMES Kah/Lrz CM lt-now
TIMES Kah CM lt-now

DELAY
OPEN Kah/Lrz WRONLY:TRUNC
TIMES Kah/Lrz CM lt-now
TIMES Kah CM lt-now

# TXT ba Hello
# WRITE Kah/Lrz ba
# SIZE Kah/Lrz 5
# DELAY
# OPEN Kah/Lrz WRONLY:TRUNC
# TIMES Kah/Lrz CM now
# TIMES Kah CM now

UNLINK Kah/Lrz
RMDIR Kah

# ---------------------------------------------------------------------------
# Open returns ENOTDIR
MKDIR Poo

CREAT Poo/Tbz
ERROR ENOTDIR
OPEN Poo/Tbz/Gnee RDONLY
OPEN Poo/Tbz/Gnee CREAT 0644
ERROR ON
UNLINK Poo/Tbz

# SYMLINK Poo/Tbz Bnz
# ERROR ENOTDIR
# OPEN Poo/Tbz/Gnee RDONLY
# OPEN Poo/Tbz/Gnee CREAT 0644
# ERROR ON
# UNLINK Poo/Tbz

RMDIR Poo

# ---------------------------------------------------------------------------
# Open returns ENAMETOOLONG
MKDIR Kah

ERROR ENAMETOOLONG
CREAT Kah/Kah_Zig_Sto_Blaz_Dru_Goz_Lrz_Poo_Tbz_Gnee_Bnz_Glap_Kah_Zig_Sto_Blaz_Dru_Goz_Lrz_Poo_Tbz_Gnee_Bnz_Glap_Kah_Zig_Sto_Blaz_Dru_Goz_Lrz_Poo_Tbz_Gnee_Bnz_Glap_Kah_Zig_Sto_Blaz_Dru_Goz_Lrz_Poo_Tbz_Gnee_Bnz_Glap_Kah_Zig_Sto_Blaz_Dru_Goz_Lrz_Poo_Tbz_Gnee_Bnz_Glap_Kah_Zig_Sto_Blaz_Dru_Goz_Lrz_Poo_Tbz_Gnee_Bnz_Glap 0644
OPEN  Kah/Kah_Zig_Sto_Blaz_Dru_Goz_Lrz_Poo_Tbz_Gnee_Bnz_Glap_Kah_Zig_Sto_Blaz_Dru_Goz_Lrz_Poo_Tbz_Gnee_Bnz_Glap_Kah_Zig_Sto_Blaz_Dru_Goz_Lrz_Poo_Tbz_Gnee_Bnz_Glap_Kah_Zig_Sto_Blaz_Dru_Goz_Lrz_Poo_Tbz_Gnee_Bnz_Glap_Kah_Zig_Sto_Blaz_Dru_Goz_Lrz_Poo_Tbz_Gnee_Bnz_Glap_Kah_Zig_Sto_Blaz_Dru_Goz_Lrz_Poo_Tbz_Gnee_Bnz_Glap CREAT:WRONLY 0644
ERROR ON

RMDIR Kah

# ---------------------------------------------------------------------------
# Open returns ENOENT
MKDIR Kah

ERROR ENOENT
OPEN Kah/Bnz/Glap CREAT:WRONLY 0644
OPEN Kah/Glap RDONLY
ERROR ON

RMDIR Kah

# ---------------------------------------------------------------------------
# Open returns EACCES
# ---------------------------------------------------------------------------
# Open returns EPERM

# ---------------------------------------------------------------------------
# Open returns ELOOP
# UMASK 022
# SETEID 0 0

# SYMLINK Kah Zig
# SYMLINK Zig Kah

# ERROR ELOOP
# CREAT Kah/Sto RDONLY
# CREAT Zig/Sto RDONLY
# ERROR ON

# UNLINK Kah
# UNLINK Zig

# ---------------------------------------------------------------------------
# Open returns EISDIR
MKDIR Bnz 0755
OPEN Bnz RDONLY
ERROR EISDIR
OPEN Bnz WRONLY
OPEN Bnz RDWR
OPEN Bnz RDONLY:TRUNC
OPEN Bnz WRONLY:TRUNC
OPEN Bnz RDWR:TRUNC
ERROR ON
RMDIR Bnz

# ---------------------------------------------------------------------------
# Open returns ELOOP when using O_NOFOLLOW
# SYMLINK Kah Zig
# ERROR
# OPEN Zig RDONLY:CREAT:NOFOLLOW 0644
# OPEN Zig RDONLY:NOFOLLOW
# OPEN Zig WRONLY:NOFOLLOW
# OPEN Zig RDWR:NOFOLLOW
# ERROR ON
# UNLINK Zig

# ---------------------------------------------------------------------------
# Open returns ENXIO (PIPE/SOCKET)
# ---------------------------------------------------------------------------
# Open returns EWOULDBLOCK
# ---------------------------------------------------------------------------
# Open returns ENOSPC
# ---------------------------------------------------------------------------
# Open returns ETXTBSY

# ---------------------------------------------------------------------------
# Open returns EEXIST
MKDIR Kah

CREAT Kah/Zig
ERROR EEXIST
OPEN Kah/Zig CREAT:EXCL 0644
ERROR ON
UNLINK Kah/Zig

# SYMLINK Kah/Zig
# ERROR EEXIST
# OPEN Kah/Zig CREAT:EXCL 0644
# ERROR ON
# UNLINK Kah/Zig

MKDIR Kah/Zig
ERROR EEXIST
OPEN Kah/Zig CREAT:EXCL 0644
ERROR ON
RMDIR Kah/Zig

RMDIR Kah

# ---------------------------------------------------------------------------
# Open returns EINVAL
# CREAT Dru 0644

# ERROR EINVAL
# OPEN Dru RDONLY:RDWWR
# OPEN Dru WRONLY:RDWWR
# OPEN Dru RDONLY:WRONLY
# OPEN Dru RDONLY:WRONLY:RDWWR
# ERROR ON

# UNLINK Dru

# ---------------------------------------------------------------------------
# ---------------------------------------------------------------------------
# Unlink removes regular files, symbolic links, fifos and sockets
UMASK 022
MKDIR Kah 0755

CREAT Kah/Zig 0644
STAT Kah/Zig REG 0644
UNLINK Kah/Zig
ERROR ENOENT
STAT Kah/Zig
ERROR ON

# symlink ${n1} Kah/Zig
# STAT Kah/Zig LNK
# UNLINK Kah/Zig
# ERROR ENOENT
# STAT Kah/Zig
# ERROR ON

# mkfifo Kah/Zig 0644
# STAT Kah/Zig FIFO
# UNLINK Kah/Zig
# ERROR ENOENT
# STAT Kah/Zig
# ERROR ON

# mknod Kah/Zig b 0644 1 2
# STAT Kah/Zig BLK
# UNLINK Kah/Zig
# ERROR ENOENT
# STAT Kah/Zig
# ERROR ON

# mknod Kah/Zig c 0644 1 2
# STAT Kah/Zig CHR
# UNLINK Kah/Zig
# ERROR ENOENT
# STAT Kah/Zig
# ERROR ON

# bind Kah/Zig
# STAT Kah/Zig SOCK
# UNLINK Kah/Zig
# ERROR ENOENT
# STAT Kah/Zig
# ERROR ON

# ---------------------------------------------------------------------------
# Successful unlink updates ctime.
# CREAT Kah/Zig 0644
# LINK Kah/Zig Sto
# TIMES Kah/Zig C now
# DELAY
# UNLINK Sto
# TIMES Kah/Zig C now
# UNLINK Kah/Zig

# ---------------------------------------------------------------------------
# Unsuccessful unlink does not update ctime.
# CREAT Kah/Zig 0644
# TIMES Kah/Zig C now
# DELAY
# SETUID 65534
# ERROR EACCES
# UNLINK Kah/Zig
# TIMES Kah/Zig C lt-now
# UNLINK Kah/Zig

MKDIR Kah/Zig 0755
CREAT Kah/Zig/Sto 0644
TIMES Kah/Zig C now
DELAY
UNLINK Kah/Zig/Sto
TIMES Kah/Zig CM lt-now
RMDIR Kah/Zig
RMDIR Kah

# ---------------------------------------------------------------------------
# Unlink returns ENOTDIR
MKDIR Kah 0755
CREAT Kah/Zig 0644
ERROR ENOTDIR
UNLINK Kah/Zig/Sto
ERROR ON
UNLINK Kah/Zig
RMDIR Kah

# ---------------------------------------------------------------------------
# Unlink returns ENAMETOOLONG
CREAT Dru 0644
UNLINK Dru
ERROR ENOENT
UNLINK Dru
ERROR ENAMETOOLONG
UNLINK Kah_Zig_Sto_Blaz_Dru_Goz_Lrz_Poo_Tbz_Gnee_Bnz_Glap_Kah_Zig_Sto_Blaz_Dru_Goz_Lrz_Poo_Tbz_Gnee_Bnz_Glap_Kah_Zig_Sto_Blaz_Dru_Goz_Lrz_Poo_Tbz_Gnee_Bnz_Glap_Kah_Zig_Sto_Blaz_Dru_Goz_Lrz_Poo_Tbz_Gnee_Bnz_Glap_Kah_Zig_Sto_Blaz_Dru_Goz_Lrz_Poo_Tbz_Gnee_Bnz_Glap_Kah_Zig_Sto_Blaz_Dru_Goz_Lrz_Poo_Tbz_Gnee_Bnz_Glap
ERROR ON

# ---------------------------------------------------------------------------
# Unlink returns ENOENT if the named file does not exist
CREAT Goz 0644
UNLINK Goz
ERROR ENOENT
UNLINK Goz
UNLINK Lrz
ERROR ON

# ---------------------------------------------------------------------------
# Unlink returns EACCES when search permission is denied for a component of the path prefix
MKDIR Kah
MKDIR Kah/Zig 0755
# CHOWN Kah/Zig 65534 65534
# SETUID 65534 65534
# CREAT Kah/Zig/Sto 0644
# CHMOD Kah/Zig 0644
# ERROR EACCES
# SETUID 65534 65534
# UNLINK Kah/Zig/Sto
# ERROR ON
# SETUID 0 0
# CHMOD Kah/Zig 0755
# SETUID 65534 65534
# UNLINK Kah/Zig/Sto
RMDIR Kah/Zig
RMDIR Kah

# ---------------------------------------------------------------------------
# Unlink returns EACCES when write permission is denied on the directory containing the link to be removed
# ---------------------------------------------------------------------------
# Unlink returns ELOOP if too many symbolic links were encountered in translating the pathname

# ---------------------------------------------------------------------------
# Unlink return EPERM if the named file is a directory
MKDIR Tbz
ERROR EPERM
UNLINK Tbz
ERROR ON
CLEAR_CACHE Tbz
ERROR EPERM
UNLINK Tbz
ERROR ON
RMDIR Tbz

# ---------------------------------------------------------------------------
# Unlink returns EPERM if the named file has its immutable, undeletable or append-only flag set

# ---------------------------------------------------------------------------
# Unlink returns EPERM if the parent directory of the named file has its immutable or append-only flag set

# ---------------------------------------------------------------------------
# Unlink returns EACCES or EPERM if the directory containing the file is marked sticky, and neither the containing directory nor the file to be removed are owned by the effective user ID

# ---------------------------------------------------------------------------
# An open file will not be immediately freed by unlink
# MKDIR Goz
# CREAT Goz/Lrz 0644
# FOPEN Fd0 Goz/Lrz WRONLY
# UNLINK Goz/Lrz
# ERROR ENOENT
# STAT Goz/Lrz
# ERROR ON
# FLINKS Fd0 0
# TXT Bf0 Hello
# FWRITE Fd0 Bf0
# FCLOSE Fd0
# ERROR ENOENT
# STAT Goz/Lrz
# ERROR ON

# ---------------------------------------------------------------------------
# ---------------------------------------------------------------------------
# Symlink creates symbolic links
# CREAT Kah 0644
# STAT Kah REG 0644
# SYMLINK Kah Zig
# STAT Zig REG 0644
# LSTAT Zig LNK
# UNLINK Kah
# ERROR ENOENT
# STAT Zig
# ERROR NO
# LSTAT Zig LNK
# UNLINK Zig

# MKDIR Sto 0755
# TIMES Sto C now
# DELAY
# SYMLINK Dru Sto/Blaz
# TIMES Sto CM now
# UNLINK Sto/Blaz
# RMDIR Sto


# ---------------------------------------------------------------------------
# ---------------------------------------------------------------------------
# Kah_Zig_Sto_Blaz_Dru_Goz_Lrz_Poo_Tbz_Gnee_Bnz_Glap

# SYMLINK
# UNLINK
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
