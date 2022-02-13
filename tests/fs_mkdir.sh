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
# Mkdir file permission
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

# Mkdir user id
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

# Mkdir file dates
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
# Mkdir returns ENOTDIR if a component of the path prefix is not a directory
UMASK 022
MKDIR Kah

CREATE Kah/Zig
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
# Mkdir returns ENAMETOOLONG  if a component of a pathname exceeded 255 characters
UMASK 022
MKDIR Kah

ERROR ENAMETOOLONG
MKDIR Kah/Kah_Zig_Sto_Blaz_Dru_Goz_Lrz_Poo_Tbz_Gnee_Bnz_Glap_Kah_Zig_Sto_Blaz_Dru_Goz_Lrz_Poo_Tbz_Gnee_Bnz_Glap_Kah_Zig_Sto_Blaz_Dru_Goz_Lrz_Poo_Tbz_Gnee_Bnz_Glap_Kah_Zig_Sto_Blaz_Dru_Goz_Lrz_Poo_Tbz_Gnee_Bnz_Glap_Kah_Zig_Sto_Blaz_Dru_Goz_Lrz_Poo_Tbz_Gnee_Bnz_Glap_Kah_Zig_Sto_Blaz_Dru_Goz_Lrz_Poo_Tbz_Gnee_Bnz_Glap
ERROR ON

RMDIR Kah

# ------------------------------------------------
# Mkdir returns ENOENT if a component of the path prefix does not exist
UMASK 022
MKDIR Kah

ERROR ENOENT
MKDIR Kah/Zig/Sto
ERROR ON

RMDIR Kah

# ------------------------------------------------
# # Mkdir returns EACCES when search permission is denied for a component of the path prefix
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
# # Mkdir returns ELOOP if too many symbolic links were encountered in translating the pathname
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
# Mkdir returns EEXIST if the named file exists

MKDIR Kah

CREATE Kah/Zig
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

