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
# truncate descrease/increase file size

CREATE Kah/Zig 0644
TRUNCATE Kah/Zig 1234567
SIZE Kah/Zig 1234567
TRUNCATE Kah/Zig 567
SIZE Kah/Zig 567
UNLINK Kah/Zig

DD /dev/random Kah/Sto 12345
SIZE Kah/Sto 12345
TRUNCATE Kah/Sto 23456
SIZE Kah/Sto 23456
TRUNCATE Kah/Sto 1
SIZE Kah/Sto 1
UNLINK Kah/Sto

# ---------------------------------------------------------------------------
# Successful truncate updates ctime
CREATE Kah/Blaz 0644
TIMES Kah/Blaz C now
DELAY
TIMES Kah/Blaz C lt-now
TRUNCATE Kah/Blaz 123
SIZE Kah/Blaz 123
TIMES Kah/Blaz C now
UNLINK Kah/Blaz

# ---------------------------------------------------------------------------
# Unsuccessful truncate does not update ctime
CREATE Kah/Dru 0444
TIMES Kah/Dru C now
DELAY
TIMES Kah/Dru C lt-now
# ERROR EACCES
# TRUNCATE Kah/Dru 123
# ERROR NO
SIZE Kah/Dru 0
TIMES Kah/Dru C lt-now
UNLINK Kah/Dru

# ---------------------------------------------------------------------------
# truncate returns ENOTDIR if a component of the path prefix is not a directory
CREATE Kah/Goz
ERROR ENOTDIR
TRUNCATE Kah/Goz/Lrz 1234
ERROR ON
UNLINK Kah/Goz

# truncate returns ENAMETOOLONG if a component of a pathname exceeded {NAME_MAX} characters
# truncate returns ENOENT if the named file does not exist
# truncate returns EACCES when search permission is denied for a component of the path prefix
# truncate returns EACCES if the named file is not writable by the user
# truncate returns ELOOP if too many symbolic links were encountered in translating the pathname
# truncate returns EPERM if the named file has its immutable or append-only flag set

# ---------------------------------------------------------------------------
# truncate returns EISDIR if the named file is a directory
ERROR EISDIR
TRUNCATE Kah 123
ERROR ON

# ---------------------------------------------------------------------------
# truncate returns ETXTBSY the file is a pure procedure (shared text) file that is being executed
# truncate returns EFBIG if the length argument was greater than the maximum file size


# ---------------------------------------------------------------------------
RMDIR Kah
