#!/usr/bin/env cli_fs
# ---------------------------------------------------------------------------
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
ERROR ON

# ---------------------------------------------------------------------------
# ---------------------------------------------------------------------------
# Unlink removes regular files, symbolic links, fifos and sockets
UMASK 022
MKDIR Kah 0755

CREATE Kah/Zig 0644
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
# CREATE Kah/Zig 0644
# LINK Kah/Zig Sto
# TIMES Kah/Zig C now
# DELAY
# UNLINK Sto
# TIMES Kah/Zig C now
# UNLINK Kah/Zig

# ---------------------------------------------------------------------------
# Unsuccessful unlink does not update ctime.
# CREATE Kah/Zig 0644
# TIMES Kah/Zig C now
# DELAY
# SETUID 65534
# ERROR EACCES
# UNLINK Kah/Zig
# TIMES Kah/Zig C lt-now
# UNLINK Kah/Zig

MKDIR Kah/Zig 0755
CREATE Kah/Zig/Sto 0644
TIMES Kah/Zig C now
DELAY
UNLINK Kah/Zig/Sto
TIMES Kah/Zig CM lt-now
RMDIR Kah/Zig
RMDIR Kah

# ---------------------------------------------------------------------------
# Unlink returns ENOTDIR
MKDIR Kah 0755
CREATE Kah/Zig 0644
ERROR ENOTDIR
UNLINK Kah/Zig/Sto
ERROR ON
UNLINK Kah/Zig
RMDIR Kah

# ---------------------------------------------------------------------------
# Unlink returns ENAMETOOLONG
CREATE Dru 0644
UNLINK Dru
ERROR ENOENT
UNLINK Dru
ERROR ENAMETOOLONG
UNLINK Kah_Zig_Sto_Blaz_Dru_Goz_Lrz_Poo_Tbz_Gnee_Bnz_Glap_Kah_Zig_Sto_Blaz_Dru_Goz_Lrz_Poo_Tbz_Gnee_Bnz_Glap_Kah_Zig_Sto_Blaz_Dru_Goz_Lrz_Poo_Tbz_Gnee_Bnz_Glap_Kah_Zig_Sto_Blaz_Dru_Goz_Lrz_Poo_Tbz_Gnee_Bnz_Glap_Kah_Zig_Sto_Blaz_Dru_Goz_Lrz_Poo_Tbz_Gnee_Bnz_Glap_Kah_Zig_Sto_Blaz_Dru_Goz_Lrz_Poo_Tbz_Gnee_Bnz_Glap
ERROR ON

# ---------------------------------------------------------------------------
# Unlink returns ENOENT if the named file does not exist
CREATE Goz 0644
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
# CREATE Kah/Zig/Sto 0644
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
# CREATE Goz/Lrz 0644
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

