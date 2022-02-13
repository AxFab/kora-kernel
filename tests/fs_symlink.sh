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
# Symlink creates symbolic links
CREATE Kah 0644
STAT Kah REG 0644
SYMLINK Kah Zig
STAT Zig REG 0644
LSTAT Zig LNK
UNLINK Kah
ERROR ENOENT
STAT Zig
ERROR ON
LSTAT Zig LNK
UNLINK Zig

MKDIR Sto 0755
TIMES Sto C now
DELAY
SYMLINK Dru Sto/Blaz
TIMES Sto CM now
UNLINK Sto/Blaz
RMDIR Sto

# ---------------------------------------------------------------------------
# symlink returns ENOTDIR if a component of the name2 path prefix is not a directory
MKDIR Dru
CREATE Dru/Goz 0644
ERROR ENOTDIR
SYMLINK Lrz Dru/Goz/Lrz
ERROR ON
UNLINK Dru/Goz
RMDIR Dru

# ---------------------------------------------------------------------------
# Symlink returns ENAMETOOLONG if a component of the name2 pathname exceeded 255 characters
SIMLINK Lrz Poo
UNLINK Poo
SIMLINK Poo Lrz
UNLINK Lrz

ERROR ENAMETOOLONG
SYMLINK Poo Kah_Zig_Sto_Blaz_Dru_Goz_Lrz_Poo_Tbz_Gnee_Bnz_Glap_Kah_Zig_Sto_Blaz_Dru_Goz_Lrz_Poo_Tbz_Gnee_Bnz_Glap_Kah_Zig_Sto_Blaz_Dru_Goz_Lrz_Poo_Tbz_Gnee_Bnz_Glap_Kah_Zig_Sto_Blaz_Dru_Goz_Lrz_Poo_Tbz_Gnee_Bnz_Glap_Kah_Zig_Sto_Blaz_Dru_Goz_Lrz_Poo_Tbz_Gnee_Bnz_Glap_Kah_Zig_Sto_Blaz_Dru_Goz_Lrz_Poo_Tbz_Gnee_Bnz_Glap
ERROR ON
SYMLINK Kah_Zig_Sto_Blaz_Dru_Goz_Lrz_Poo_Tbz_Gnee_Bnz_Glap_Kah_Zig_Sto_Blaz_Dru_Goz_Lrz_Poo_Tbz_Gnee_Bnz_Glap_Kah_Zig_Sto_Blaz_Dru_Goz_Lrz_Poo_Tbz_Gnee_Bnz_Glap_Kah_Zig_Sto_Blaz_Dru_Goz_Lrz_Poo_Tbz_Gnee_Bnz_Glap_Kah_Zig_Sto_Blaz_Dru_Goz_Lrz_Poo_Tbz_Gnee_Bnz_Glap_Kah_Zig_Sto_Blaz_Dru_Goz_Lrz_Poo_Tbz_Gnee_Bnz_Glap Poo
UNLINK Poo

# ---------------------------------------------------------------------------
# Symlink returns ENOENT if a component of the name2 path prefix does not exist
MKDIR Tbz 0755
ERROR ENOENT
SYMLINK Bnz Tbz/Gnee/Bnz
ERROR ON
RMDIR Tbz

# ---------------------------------------------------------------------------
# Symlink returns EACCES when a component of the name2 path prefix denies search permission

