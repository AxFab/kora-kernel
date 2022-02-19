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
# Link creates hardlinks
MKDIR Kah 0644

CREATE Kah/Zig
STAT Kah/Zig REG
LINKS Kah/Zig 1

LINK Kah/Sto Kah/Zig
STAT Kah/Sto REG
LINKS Kah/Zig 2
LINKS Kah/Sto 2

LINK Kah/Dru Kah/Sto
STAT Kah/Dru REG
LINKS Kah/Zig 3
LINKS Kah/Sto 3
LINKS Kah/Dru 3

CHMOD Kah/Sto 0201
CHOWN Kah/Sto 65534 65533
STAT Kah/Zig REG 0201 65534 65533
STAT Kah/Sto REG 0201 65534 65533
STAT Kah/Dru REG 0201 65534 65533
LINKS Kah/Zig 3
LINKS Kah/Sto 3
LINKS Kah/Dru 3

UNLINK Kah/Zig
ERROR ENOENT
STAT Kah/Zig
ERROR ON
STAT Kah/Sto REG 0201 65534 65533
STAT Kah/Dru REG 0201 65534 65533
LINKS Kah/Sto 2
LINKS Kah/Dru 2

UNLINK Kah/Sto
ERROR ENOENT
STAT Kah/Zig
STAT Kah/Sto
ERROR ON
STAT Kah/Dru REG 0201 65534 65533
LINKS Kah/Dru 1

UNLINK Kah/Dru
ERROR ENOENT
STAT Kah/Zig
STAT Kah/Sto
STAT Kah/Dru
ERROR ON



RMDIR Kah

# Kah Zig Sto Blaz Dru Goz Lrz Poo Tbz Gnee Bnz Glap

