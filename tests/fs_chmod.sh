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
UMASK 022
MKDIR Kah

# ---------------------------------------------------------------------------
# chmod changes permission
CREATE Kah/Zig
CHMOD Kah/Zig 0111
STAT Kah/Zig 0111

SYMLINK Zig Kah/Sto
LS Kah
LSTAT Kah/Sto 0644
CHMOD Kah/Sto 0222
STAT Kah/Zig 0222
STAT Kah/Sto 0222
LSTAT Kah/Sto 0644

UNLINK Kah/Zig
UNLINK Kah/Sto


MKDIR Kah/Zig
CHMOD Kah/Zig 0111
STAT Kah/Zig 0111

SYMLINK Zig Kah/Sto
LSTAT Kah/Sto 0644
CHMOD Kah/Sto 0222
STAT Kah/Zig 0222
STAT Kah/Sto 0222
LSTAT Kah/Sto 0644

RMDIR Kah/Zig
UNLINK Kah/Sto

# ---------------------------------------------------------------------------
# Successful chmod updates ctime

# ---------------------------------------------------------------------------
# Unsuccessful chmod does not updates ctime

# chmod returns ENOTDIR if a component of the path prefix is not a directory
# chmod returns ENAMETOOLONG if a component of a pathname exceeded the maximum of allowed characters
# chmod returns ENOENT if the named file does not exist
# chmod returns EACCES when search permission is denied for a component of the path prefix
# chmod returns ELOOP if too many symbolic links were encountered in translating the pathname
# chmod returns EPERM if the operation would change the ownership, but the effective user ID is not the super-user
# chmod returns EPERM if the named file has its immutable or append-only flag set
# chmod returns EFTYPE if the effective user ID is not the super-user, the mode includes the sticky bit (S_ISVTX), and path does not refer to a directory
# Verify SUID/SGID bit behaviour

# ---------------------------------------------------------------------------
RMDIR Kah

# Kah Zig Sto Blaz Dru Goz Lrz Poo Tbz Gnee Bnz Glap

