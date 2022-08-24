#!/usr/bin/env cli_fs
# ---------------------------------------------------------------------------
# Highly inspirated from `pjdfstest` testing program
# ---------------------------------------------------------------------------
# License for all REGression tests available with pjdfstest:
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
# rename changes file name

CREATE Kah 0644
LSTAT Kah REG 0644
LINKS Kah 1
INO Kah - @i1
RENAME Kah Zig
ERROR ENOENT
LSTAT Kah REG 0644
ERROR ON
LSTAT Zig REG 0644
LINKS Zig 1
INO Zig @i1
LINK Zig Kah
LSTAT Kah REG 0644
LINKS Kah 2
INO Kah @i1
LSTAT Zig REG 0644
LINKS Zig 2
INO Zig @i1
CLEAR_CACHE Kah
LS .
UNLINK Kah
LS .
UNLINK Zig


MKDIR Sto 0755
LSTAT Sto DIR 0755
LINKS Sto 2
INO Sto - @i2
RENAME Sto Blaz
ERROR ENOENT
LSTAT Sto DIR 0755
ERROR ON
LSTAT Blaz DIR 0755
LINKS Blaz 2
RMDIR Blaz

# ---------------------------------------------------------------------------
# rename returns ENAMETOOLONG if a component of either pathname exceeded {NAME_MAX} characters
# rename returns ENOENT if a component of the 'from' path does not exist, or a path prefix of 'to' does not exist
# rename returns EACCES when a component of either path prefix denies search permission
# rename returns EACCES when the requested link requires writing in a directory with a mode that denies write permission
# rename returns EPERM if the file pointed at by the 'from' argument has its immutable, undeletable or append-only flag set
# rename returns EPERM if the parent directory of the file pointed at by the 'from' argument has its immutable or append-only flag set
# rename returns EPERM if the parent directory of the file pointed at by the 'to' argument has its immutable flag set
# rename returns EACCES or EPERM if the directory containing 'from' is marked sticky, and neither the containing directory nor 'from' are owned by the effective user ID
# rename returns EACCES or EPERM if the file pointed at by the 'to' argument exists, the directory containing 'to' is marked sticky, and neither the containing directory nor 'to' are owned by the effective user ID
# rename returns ELOOP if too many symbolic links were encountered in translating one of the pathnames
# rename returns ENOTDIR if a component of either path prefix is not a directory
# rename returns ENOTDIR when the 'from' argument is a directory, but 'to' is not a directory
# rename returns EISDIR when the 'to' argument is a directory, but 'from' is not a directory
# rename returns EXDEV if the link named by 'to' and the file named by 'from' are on different file systems
# rename returns EINVAL when the 'from' argument is a parent directory of 'to'
# rename returns EINVAL/EBUSY when an attempt is made to rename '.' or '..'
# rename returns EEXIST or ENOTEMPTY if the 'to' argument is a directory and is not empty
# write access to subdirectory is required to move it to another directory
# rename changes file ctime
# unsuccefull rename does not changes file ctime
# rename succeeds when to is multiply linked
# rename of a directory updates its .. link

# ---------------------------------------------------------------------------
