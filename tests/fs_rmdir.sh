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
# Rmdir removes directories
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
# Rmdir returns ENOTDIR if a component of the path is not a directory
MKDIR Kah 0755
CREATE Kah/Zig 0644
ERROR ENOTDIR
RMDIR Kah/Zig/Sto
ERROR ON
UNLINK Kah/Zig
RMDIR Kah

CREATE Sto 0644
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
# Rmdir returns ENAMETOOLONG
MKDIR Kah 0755
RMDIR Kah

ERROR ENOENT
RMDIR Kah

ERROR ENAMETOOLONG
MKDIR Kah_Zig_Sto_Blaz_Dru_Goz_Lrz_Poo_Tbz_Gnee_Bnz_Glap_Kah_Zig_Sto_Blaz_Dru_Goz_Lrz_Poo_Tbz_Gnee_Bnz_Glap_Kah_Zig_Sto_Blaz_Dru_Goz_Lrz_Poo_Tbz_Gnee_Bnz_Glap_Kah_Zig_Sto_Blaz_Dru_Goz_Lrz_Poo_Tbz_Gnee_Bnz_Glap_Kah_Zig_Sto_Blaz_Dru_Goz_Lrz_Poo_Tbz_Gnee_Bnz_Glap_Kah_Zig_Sto_Blaz_Dru_Goz_Lrz_Poo_Tbz_Gnee_Bnz_Glap
ERROR ON

# ------------------------------------------------
# Rmdir returns ENOENT
MKDIR Kah 0755
RMDIR Kah

ERROR ENOENT
RMDIR Kah
RMDIR Zig
ERROR ON

# ------------------------------------------------
# Rmdir returns ELOOP
# SYMLINK Kah Zig
# SYMLINK Zig Kah

# ERROR ELOOP
# RMDIR Kah/Sto
# RMDIR Zig/Sto
# ERROR ON

# UNLINK Kah
# UNLINK Zig

# ------------------------------------------------
# Rmdir returns ENOTEMPTY
MKDIR Kah

CREATE Kah/Zig
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
# Rmdir returns EACCES


# ------------------------------------------------
# Rmdir returns EPERM

# ------------------------------------------------
# Rmdir returns EINVAL
# MKDIR Kah
# MKDIR Kah/Zig
# ERROR EINVAL
# RMDIR Kah/Zig/.
# RMDIR Kah/Zig/..
# ERROR ON
# RMDIR Kah/Zig
# RMDIR Kah


# ------------------------------------------------
# Rmdir returns EBUSY
# MKDIR Kah
# # MKDIR Kah/Zig
# MOUNT - devfs Kah/Zig
# ERROR EBUSY
# RMDIR Kah/Zig
# ERROR ON
# UMOUNT Kah/Zig
# # RMDIR Kah/Zig
# RMDIR Kah

