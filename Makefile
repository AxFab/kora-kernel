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

# -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

CFLAGS += -Wall -Wextra -Wno-unused-parameter
CFLAGS += -ffreestanding
CFLAGS += -I$(topdir)/include
CFLAGS += -I$(topdir)/arch/$(target_arch)/include
CFLAGS += -ggdb
CFLAGS += -D_DATE_=\"'$(DATE)'\" -D_OSNAME_=\"'$(LINUX)'\"
CFLAGS += -D_GITH_=\"'$(GIT)'\" -D_VTAG_=\"'$(VERSION)'\"
ifneq ($(target_arch),_simu)
ifeq ($(target_os),kora)
CFLAGS += -DKORA_KRN -D__NO_SYSCALL
endif
endif
ifneq ($(target_os),kora)
ifeq ($(NOCOV),)
CFLAGS += --coverage -fprofile-arcs -ftest-coverage
endif
endif


include $(topdir)/make/build.mk

# -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

SRCS-y += $(wildcard $(srcdir)/basic/*.c)
SRCS-y += $(wildcard $(srcdir)/core/*.c)
SRCS-y += $(wildcard $(srcdir)/files/*.c)
SRCS-y += $(wildcard $(srcdir)/mem/*.c)
SRCS-y += $(wildcard $(srcdir)/net/*.c)
SRCS-y += $(wildcard $(srcdir)/net/ip4/*.c)
SRCS-y += $(wildcard $(srcdir)/task/*.c)
SRCS-y += $(wildcard $(srcdir)/vfs/*.c)
SRCS-y += $(wildcard $(arcdir)/*.$(ASM_EXT))
SRCS-y += $(wildcard $(arcdir)/*.c)
ifneq ($(target_arch),_simu)
SRCS-y += $(wildcard $(srcdir)/stdc/*.c)
SRCS-y += $(wildcard $(srcdir)/misc/*.c)
else
SRCS-y += $(srcdir)/stdc/mtx.c
SRCS-y += $(srcdir)/stdc/cnd.c
SRCS-y += $(srcdir)/tests/thrd.c
endif
SRCS-y += # Drivers

include $(topdir)/arch/$(target_arch)/make.mk

$(bindir)/$(kname): $(call fn_objs,SRCS-y)
	$(S) mkdir -p $(dir $@)
	$(Q) echo "    LD  "$@
ifneq ($(target_arch),_simu)
	$(V) $(CC) -T $(arcdir)/kernel.ld -o $@ $^ -nostdlib -lgcc
else
	$(V) $(CC) -o $@ $^ -latomic -lpthread
endif

# -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

CHECKS += cksync ckutils
CHECKS += ckpipe ckblk ckgfx # Files
CHECKS += ckelf ckmem cknet
CHECKS += ckvfs
CHECKS += ckkrn # No args, put all files on coverage

include $(topdir)/make/check.mk

ifeq ($(NOCOV),)
CKLFLGS += --coverage -fprofile-arcs -ftest-coverage
endif
CKLFLGS += -lpthread
ifeq ($(USE_ATOMIC),y)
CKLFLGS = -latomic
endif

cksync_src-y += $(srcdir)/basic/futex.c
cksync_src-y += $(srcdir)/basic/bbtree.c
cksync_src-y += $(srcdir)/basic/hmap.c
cksync_src-y += $(srcdir)/stdc/cnd.c
cksync_src-y += $(srcdir)/stdc/mtx.c
cksync_src-y += $(srcdir)/tests/stub.c
cksync_src-y += $(srcdir)/tests/thrd.c
cksync_src-y += $(srcdir)/tests/sched.c
cksync_src-y += $(srcdir)/tests/tst_sync.c
$(eval $(call link_bin,cksync,cksync_src,CKLFLGS))

ckutils_src-y += $(srcdir)/basic/bbtree.c
ckutils_src-y += $(srcdir)/basic/hmap.c
ckutils_src-y += $(srcdir)/tests/stub.c
ckutils_src-y += $(srcdir)/tests/tst_utils.c
$(eval $(call link_bin,ckutils,ckutils_src,CKLFLGS))

ckpipe_src-y += $(srcdir)/basic/futex.c
ckpipe_src-y += $(srcdir)/basic/bbtree.c
ckpipe_src-y += $(srcdir)/files/pipe.c
ckpipe_src-y += $(srcdir)/stdc/cnd.c
ckpipe_src-y += $(srcdir)/stdc/mtx.c
ckpipe_src-y += $(srcdir)/tests/stub.c
ckpipe_src-y += $(srcdir)/tests/thrd.c
ckpipe_src-y += $(srcdir)/tests/sched.c
ckpipe_src-y += $(srcdir)/tests/tst_pipe.c
$(eval $(call link_bin,ckpipe,ckpipe_src,CKLFLGS))

ckblk_src-y += $(srcdir)/basic/bbtree.c
ckblk_src-y += $(srcdir)/basic/futex.c
ckblk_src-y += $(srcdir)/stdc/cnd.c
ckblk_src-y += $(srcdir)/stdc/mtx.c
ckblk_src-y += $(srcdir)/files/blk.c
ckblk_src-y += $(srcdir)/tests/stub.c
ckblk_src-y += $(srcdir)/tests/thrd.c
ckblk_src-y += $(srcdir)/tests/sched.c
ckblk_src-y += $(srcdir)/tests/tst_blk.c
$(eval $(call link_bin,ckblk,ckblk_src,CKLFLGS))

ckgfx_src-y += $(srcdir)/files/gfx.c
ckgfx_src-y += $(srcdir)/tests/stub.c
ckgfx_src-y += $(srcdir)/tests/tst_gfx.c
$(eval $(call link_bin,ckgfx,ckgfx_src,CKLFLGS))

cknet_src-y += $(srcdir)/basic/futex.c
cknet_src-y += $(srcdir)/basic/bbtree.c
cknet_src-y += $(srcdir)/core/debug.c
cknet_src-y += $(srcdir)/files/pipe.c
cknet_src-y += $(srcdir)/stdc/cnd.c
cknet_src-y += $(srcdir)/stdc/mtx.c
cknet_src-y += $(srcdir)/net/net.c
cknet_src-y += $(srcdir)/net/socket.c
cknet_src-y += $(srcdir)/net/local.c
cknet_src-y += $(srcdir)/tests/stub.c
cknet_src-y += $(srcdir)/tests/thrd.c
cknet_src-y += $(srcdir)/tests/sched.c
cknet_src-y += $(srcdir)/tests/tst_net.c
$(eval $(call link_bin,cknet,cknet_src,CKLFLGS))

ckelf_src-y += $(srcdir)/core/elf.c
# ckelf_src-y += $(srcdir)/core/dlib.c
ckelf_src-y += $(srcdir)/basic/hmap.c
ckelf_src-y += $(srcdir)/tests/stub.c
ckelf_src-y += $(srcdir)/tests/tst_elf.c
$(eval $(call link_bin,ckelf,ckelf_src,CKLFLGS))

ckmem_src-y += $(wildcard $(srcdir)/mem/*.c)
ckmem_src-y += $(srcdir)/basic/bbtree.c
ckmem_src-y += $(srcdir)/core/debug.c
ckmem_src-y += $(srcdir)/tests/stub.c
ckmem_src-y += $(srcdir)/tests/tst_mem.c
ckmem_src-y += $(topdir)/arch/_simu/mmu.c
$(eval $(call link_bin,ckmem,ckmem_src,CKLFLGS))

ckvfs_src-y += $(srcdir)/basic/bbtree.c
ckvfs_src-y += $(srcdir)/basic/hmap.c
ckvfs_src-y += $(srcdir)/basic/futex.c
ckvfs_src-y += $(srcdir)/core/debug.c
ckvfs_src-y += $(wildcard $(srcdir)/vfs/*.c)
ckvfs_src-y += $(srcdir)/tests/stub.c
ckvfs_src-y += $(srcdir)/tests/thrd.c
ckvfs_src-y += $(srcdir)/tests/sched.c
ckvfs_src-y += $(srcdir)/tests/tst_vfs.c
ckvfs_src-y += $(topdir)/arch/_simu/hostfs.c
$(eval $(call link_bin,ckvfs,ckvfs_src,CKLFLGS))

ckkrn_src-y += $(wildcard $(srcdir)/basic/*.c)
ckkrn_src-y += $(wildcard $(srcdir)/core/*.c)
ckkrn_src-y += $(wildcard $(srcdir)/files/*.c)
ckkrn_src-y += $(wildcard $(srcdir)/mem/*.c)
ckkrn_src-y += $(wildcard $(srcdir)/net/*.c)
ckkrn_src-y += $(wildcard $(srcdir)/net/ip4/*.c)
ckkrn_src-y += $(wildcard $(srcdir)/task/*.c)
ckkrn_src-y += $(wildcard $(srcdir)/vfs/*.c)
ckkrn_src-y += $(wildcard $(topdir)/arch/_simu/*.c)
ckkrn_src-y += $(srcdir)/stdc/mtx.c
ckkrn_src-y += $(srcdir)/stdc/cnd.c
ckkrn_src-y += $(srcdir)/tests/thrd.c
$(eval $(call link_bin,ckkrn,ckkrn_src,CKLFLGS))


ifeq ($(NODEPS),)
# -include $(call fn_deps,SRCS-y)
endif
