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

CREATE Poo/Tbz
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
CREATE Kah/Kah_Zig_Sto_Blaz_Dru_Goz_Lrz_Poo_Tbz_Gnee_Bnz_Glap_Kah_Zig_Sto_Blaz_Dru_Goz_Lrz_Poo_Tbz_Gnee_Bnz_Glap_Kah_Zig_Sto_Blaz_Dru_Goz_Lrz_Poo_Tbz_Gnee_Bnz_Glap_Kah_Zig_Sto_Blaz_Dru_Goz_Lrz_Poo_Tbz_Gnee_Bnz_Glap_Kah_Zig_Sto_Blaz_Dru_Goz_Lrz_Poo_Tbz_Gnee_Bnz_Glap_Kah_Zig_Sto_Blaz_Dru_Goz_Lrz_Poo_Tbz_Gnee_Bnz_Glap 0644
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
# CREATE Kah/Sto RDONLY
# CREATE Zig/Sto RDONLY
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

CREATE Kah/Zig
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
# CREATE Dru 0644

# ERROR EINVAL
# OPEN Dru RDONLY:RDWWR
# OPEN Dru WRONLY:RDWWR
# OPEN Dru RDONLY:WRONLY
# OPEN Dru RDONLY:WRONLY:RDWWR
# ERROR ON

# UNLINK Dru
