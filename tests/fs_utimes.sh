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
# utimes changes timestamps on any type of file
CREATE Kah
UTIMES Kah 1900000000 A  # Sun Mar 17 11:46:40 MDT 2030
UTIMES Kah 1950000000 M  # Fri Oct 17 04:40:00 MDT 2031
STAT Kah
TIMES Kah A 1900000000
TIMES Kah M 1950000000
UNLINK Kah

MKDIR Zig
UTIMES Zig 1900000000 A  # Sun Mar 17 11:46:40 MDT 2030
UTIMES Zig 1950000000 M  # Fri Oct 17 04:40:00 MDT 2031
STAT Zig
TIMES Zig A 1900000000
TIMES Zig M 1950000000
RMDIR Zig


# ---------------------------------------------------------------------------
# utimes with UTIME_NOW will set the will set typestamps to now
# utimes with UTIME_OMIT will leave the time unchanged
# utimes can update birthtimes
# utimes can set mtime < atime or vice versa
# utimes can follow symlinks
# utimes with UTIME_NOW will work if the caller has write permission
# utimes will work if the caller is the owner or root
# utimes can set timestamps with subsecond precision
# utimes is y2038 compliant

# ---------------------------------------------------------------------------
