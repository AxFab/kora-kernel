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
MKDIR Kah 0755

# ---------------------------------------------------------------------------
# ---------------------------------------------------------------------------
# Link creates hardlinks

CREATE Kah/Zig
STAT Kah/Zig REG
LINKS Kah/Zig 1

LINK Kah/Zig Kah/Sto
STAT Kah/Sto REG
LINKS Kah/Zig 2
LINKS Kah/Sto 2

LINK Kah/Sto Kah/Dru
STAT Kah/Dru REG
LINKS Kah/Zig 3
LINKS Kah/Sto 3
LINKS Kah/Dru 3

# CHMOD Kah/Sto 0201
# CHOWN Kah/Sto 65534 65533
# STAT Kah/Zig REG 0201 65534 65533
# STAT Kah/Sto REG 0201 65534 65533
# STAT Kah/Dru REG 0201 65534 65533
# LINKS Kah/Zig 3
# LINKS Kah/Sto 3
# LINKS Kah/Dru 3

UNLINK Kah/Zig
ERROR ENOENT
STAT Kah/Zig
ERROR ON
STAT Kah/Sto REG # 0201 65534 65533
STAT Kah/Dru REG # 0201 65534 65533
LINKS Kah/Sto 2
LINKS Kah/Dru 2

UNLINK Kah/Sto
ERROR ENOENT
STAT Kah/Zig
STAT Kah/Sto
ERROR ON
STAT Kah/Dru REG # 0201 65534 65533
LINKS Kah/Dru 1

UNLINK Kah/Dru
ERROR ENOENT
STAT Kah/Zig
STAT Kah/Sto
STAT Kah/Dru
ERROR ON

# ---------------------------------------------------------------------------
# Link update ctime
CREATE Kah/Goz
STAT Kah/Goz REG
LINKS Kah/Goz 1

DELAY
TIMES Kah/Goz C lt-now
LINK Kah/Goz Kah/Lrz
TIMES Kah/Goz C now

# DELAY
# TIMES Kah/Goz C lt-now
# SET EUI
# ERROR EACCESS
# LINK Kah/Goz Kah/Lrz
# ERROR ON
# TIMES Kah/Goz C lt-now

UNLINK Kah/Goz
UNLINK Kah/Lrz

# ---------------------------------------------------------------------------
# Link returns ENOTDIR if a component of either path prefix is not a directory
CREATE Kah/Poo
CREATE Kah/Tbz
ERROR ENOTDIR
LINK Kah/Poo/Bnz Kah/Bnz
LINK Kah/Tbz Kah/Poo/Bnz
ERROR ON
UNLINK Kah/Poo
UNLINK Kah/Tbz

# ---------------------------------------------------------------------------
# link returns ENAMETOOLONG if a component of either pathname exceeded {NAME_MAX} characters
CREATE Kah/Glap
ERROR ENAMETOOLONG
LINK Kah/Glap Kah/Kah_Zig_Sto_Blaz_Dru_Goz_Lrz_Poo_Tbz_Gnee_Bnz_Glap_Kah_Zig_Sto_Blaz_Dru_Goz_Lrz_Poo_Tbz_Gnee_Bnz_Glap_Kah_Zig_Sto_Blaz_Dru_Goz_Lrz_Poo_Tbz_Gnee_Bnz_Glap_Kah_Zig_Sto_Blaz_Dru_Goz_Lrz_Poo_Tbz_Gnee_Bnz_Glap_Kah_Zig_Sto_Blaz_Dru_Goz_Lrz_Poo_Tbz_Gnee_Bnz_Glap_Kah_Zig_Sto_Blaz_Dru_Goz_Lrz_Poo_Tbz_Gnee_Bnz_Glap
LINK Kah/Kah_Zig_Sto_Blaz_Dru_Goz_Lrz_Poo_Tbz_Gnee_Bnz_Glap_Kah_Zig_Sto_Blaz_Dru_Goz_Lrz_Poo_Tbz_Gnee_Bnz_Glap_Kah_Zig_Sto_Blaz_Dru_Goz_Lrz_Poo_Tbz_Gnee_Bnz_Glap_Kah_Zig_Sto_Blaz_Dru_Goz_Lrz_Poo_Tbz_Gnee_Bnz_Glap_Kah_Zig_Sto_Blaz_Dru_Goz_Lrz_Poo_Tbz_Gnee_Bnz_Glap_Kah_Zig_Sto_Blaz_Dru_Goz_Lrz_Poo_Tbz_Gnee_Bnz_Glap Kah/Glap
ERROR ON
UNLINK Kah/Glap

# ---------------------------------------------------------------------------
# link returns ENOENT if a component of either path prefix does not exist
MKDIR Gnee
CREATE Gnee/Zig

ERROR ENOENT
LINK Gnee/Poo/Glap Bnz
LINK Gnee/Zig Gnee/Poo/Glap
ERROR ON

UNLINK Gnee/Zig
RMDIR Gnee

# ---------------------------------------------------------------------------
# link returns EMLINK if the link count of the file would exceed 32767

# ---------------------------------------------------------------------------
# link returns EACCES when a component of either path prefix denies search permission

# ---------------------------------------------------------------------------
# link returns EACCES when the requested link requires writing in a directory with a mode that denies write permission

# ---------------------------------------------------------------------------
# link returns ELOOP if too many symbolic links were encountered in translating one of the pathnames
LS .
SYMLINK Blaz Dru
SYMLINK Dru Blaz
CREATE Lrz
ERROR ELOOP
LINK Blaz/Goz Goz
LINK Dru/Goz Goz
LINK Lrz Blaz/Goz
LINK Lrz Dru/Goz
ERROR ON
UNLINK Blaz
UNLINK Dru
UNLINK Lrz

# ---------------------------------------------------------------------------
# link returns ENOENT if the source file does not exist
CREATE Sto
LINK Sto Bnz
UNLINK Sto
UNLINK Bnz
ERROR ENOENT
LINK Sto Bnz
ERROR ON

# ---------------------------------------------------------------------------
# link returns EEXIST if the destination file does exist
CREATE Sto

CREATE Bnz
ERROR EEXIST
LINK Sto Bnz
ERROR ON
UNLINK Bnz

MKDIR Bnz
ERROR EEXIST
LINK Sto Bnz
ERROR ON
RMDIR Bnz

UNLINK Sto

# ---------------------------------------------------------------------------
# link returns EPERM if the source file is a directory
MKDIR Kah/Blaz
ERROR EPERM
LINK Kah/Blaz Zig
ERROR ON
RMDIR Kah/Blaz

# ---------------------------------------------------------------------------
# link returns EPERM if the source file has its immutable or append-only flag set

# ---------------------------------------------------------------------------
# link returns EPERM if the parent directory of the destination file has its immutable flag set

# ---------------------------------------------------------------------------
# link returns EXDEV if the source and the destination files are on different file systems
ERROR EXDEV
LINK dev/null Kah/Bnz
ERROR ON

# ---------------------------------------------------------------------------
# link returns ENOSPC if the directory in which the entry for the new link is being placed cannot be extended because there is no space left on the file system containing the directory



# ---------------------------------------------------------------------------
RMDIR Kah
