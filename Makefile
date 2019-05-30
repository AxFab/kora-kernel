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
#  This makefile is more or less generic.
# -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
topdir ?= $(shell readlink -f $(dir $(word 1,$(MAKEFILE_LIST))))
gendir ?= $(shell pwd)

include $(topdir)/make/global.mk
ASM_EXT := asm
srcdir = $(topdir)/src
arcdir = $(topdir)/arch/$(target_arch)
kname = kora-$(target_arch).krn

all: kernel
kernel: $(bindir)/$(kname)

install: $(bindir)/$(kname)

check: $(bindir)/ktest

CFLAGS += -Wall -Wextra -Wno-unused-parameter
CFLAGS += -ffreestanding
CFLAGS += -I$(topdir)/include
CFLAGS += -I$(topdir)/arch/$(target_arch)/include
CFLAGS += -ggdb
CFLAGS += -DKORA_STDC -DKORA_KRN -D__NO_SYSCALL
CFLAGS += -D_DATE_=\"'$(DATE)'\" -D_OSNAME_=\"'$(LINUX)'\"
CFLAGS += -D_GITH_=\"'$(GIT)'\" -D_VTAG_=\"'$(VERSION)'\"

include $(topdir)/make/build.mk

SRCS-y += $(wildcard $(srcdir)/core/*.c)
SRCS-y += $(wildcard $(srcdir)/files/*.c)
SRCS-y += $(wildcard $(srcdir)/task/*.c)
SRCS-y += $(wildcard $(srcdir)/mem/*.c)
SRCS-y += $(wildcard $(srcdir)/vfs/*.c)
SRCS-y += $(wildcard $(srcdir)/net/*.c)
SRCS-y += $(wildcard $(arcdir)/kernel/*.$(ASM_EXT))
SRCS-y += $(wildcard $(arcdir)/kernel/*.c)
SRCS-y += $(wildcard $(srcdir)/basic/*.c)
# SRCS-y += $(wildcard $(srcdir)/stdc/*.c)
SRCS-y += $(srcdir)/stdc/mtx.c
SRCS-y += $(srcdir)/stdc/cnd.c
SRCS-y += # Drivers

include $(topdir)/arch/$(target_arch)/make.mk

$(bindir)/$(kname): $(call fn_objs,SRCS-y)
	$(S) mkdir -p $(dir $@)
	$(Q) echo "    LD  "$@
	$(V) $(CC) -o $@ $^ -latomic -lpthread
# $(V) $(CC) -T $(arcdir)/kernel.ld -o $@ $^ -nostdlib -lgcc -latomic -pthread

CKSRCS-y += $(srcdir)/basic/futex.c
CKSRCS-y += $(srcdir)/basic/bbtree.c
CKSRCS-y += $(srcdir)/basic/hmap.c
CKSRCS-y += $(srcdir)/stdc/cnd.c
CKSRCS-y += $(srcdir)/stdc/mtx.c
CKSRCS-y += $(srcdir)/test.c
CKSRCS-y += $(srcdir)/thrd.c
CKSRCS-y += $(srcdir)/sched.c
CKSRCS-y += $(srcdir)/tst_sync.c
# CKSRCS-y += $(srcdir)/atomic_arm.c

CKLFLGS = -lpthread
# CKLFLGS = -latomic

$(eval $(call link_bin,ktest,CKSRCS,CKLFLGS))

ifeq ($(NODEPS),)
include $(call fn_deps,SRCS)
endif
