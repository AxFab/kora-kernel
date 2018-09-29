#      This file is part of the KoraOS project.
#  Copyright (C) 2018  <Fabien Bavent>
#
#  This program is free software: you can redistribute it and/or modify
#  it under the terms of the GNU Affero General Public License as
#  published by the Free Software Foundation, either version 3 of the
#  License, or (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU Affero General Public License for more details.
#
#  You should have received a copy of the GNU Affero General Public License
#  along with this program.  If not, see <http://www.gnu.org/licenses/>.
# -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
NAME=kora-kernel
VERSION=0.0-$(GIT)

CFLAGS += -Wall -Wextra -Wno-unused-parameter -fno-builtin
CFLAGS += -D_DATE_=\"'$(DATE)'\" -D_OSNAME_=\"'$(LINUX)'\"
CFLAGS += -D_GITH_=\"'$(GIT)'\" -D_VTAG_=\"'$(VERSION)'\"
CFLAGS += -Wno-multichar -Wno-implicit-fallthrough
CFLAGS += -ggdb3 -I$(topdir)/include

COV_FLAGS += --coverage -fprofile-arcs -ftest-coverage
KRN_FLAGS += -DKORA_STDC
# KRN_FLAGS += -DSPLOCK_TICKET

# -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
# We define modes of compiling
std_CFLAGS := $(CFLAGS) -I$(topdir)/include/arch/x86-gcc
std_CFLAGS += -D_FAKE_MEM -D_FAKE_TIME -D_FAKE_TASK
# $(COV_FLAGS)
krn_CFLAGS := $(CFLAGS) -I$(topdir)/include/arch/$(target_arch) $(KRN_FLAGS)
$(eval $(call ccpl,std))
$(eval $(call ccpl,krn))



# -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
# We create the `tsVfs` delivery
tsVfs_src-y += $(srcdir)/core/bio.c $(srcdir)/core/debug.c $(srcdir)/core/irq.c
tsVfs_src-y += $(srcdir)/libc/bbtree.c $(srcdir)/libc/hmap.c $(srcdir)/libc/random.c
tsVfs_src-y += $(srcdir)/libc/string.c $(srcdir)/libc/time.c
tsVfs_src-y += $(srcdir)/files/ioblk.c  $(srcdir)/task/async.c
tsVfs_src-y += $(wildcard $(srcdir)/vfs/*.c)
tsVfs_src-y += $(wildcard $(srcdir)/drivers/disk/imgdk/*.c)
tsVfs_src-y += $(wildcard $(srcdir)/drivers/fs/fat/*.c)
tsVfs_src-y += $(srcdir)/../vs/core.c
tsVfs_src-y += $(srcdir)/../vs/test_vfs.c


$(eval $(call link,tsVfs,std))
DV_UTILS += $(bindir)/tsVfs

