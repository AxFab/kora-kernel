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

include $(topdir)/var/make/global.mk
ASM_EXT := asm
srcdir = $(topdir)/src
arcdir = $(topdir)/arch/$(target_arch)
kname = kora-$(target_arch).krn

all: kernel

kernel: $(bindir)/$(kname)

install: $(bindir)/$(kname)

# -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

CFLAGS ?= -Wall -Wextra -Wno-unused-parameter -ggdb
CFLAGS += -ffreestanding
CFLAGS += -I$(topdir)/include
CFLAGS += -I$(topdir)/src/_$(target_os)/include
CFLAGS += -I$(topdir)/arch/$(target_arch)/include
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


include $(topdir)/var/make/build.mk

# -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

SRCS-y += $(wildcard $(srcdir)/basic/*.c)
SRCS-y += $(wildcard $(srcdir)/core/*.c)
SRCS-y += $(wildcard $(srcdir)/files/*.c)
SRCS-y += $(wildcard $(srcdir)/mem/*.c)
SRCS-y += $(wildcard $(srcdir)/net/*.c)
# SRCS-y += $(wildcard $(srcdir)/net/ip4/*.c)
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

SRCS-y += $(srcdir)/core/futex.c
SRCS-y += $(srcdir)/misc/scheduler.c

include $(topdir)/arch/$(target_arch)/make.mk

$(bindir)/$(kname): $(call fn_objs,SRCS-y)
	$(S) mkdir -p $(dir $@)
	$(Q) echo "    LD  "$@
ifeq ($(target_arch),_simu)
	$(V) $(LDC) -o $@ $^ -latomic -lpthread
else
ifeq ($(target_arch),blank)
	$(V) $(LDC) -o $@ $^ -latomic -lpthread
else
	$(V) $(CC) -T $(arcdir)/kernel.ld -o $@ $^ -nostdlib -lgcc
endif
endif

# -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

# CHECKS += cksync
CHECKS += ckutils
CHECKS += ckpipe ckblk ckgfx ckwin # Files
CHECKS += ckelf ckmem cknet
CHECKS += ckvfs
CHECKS += ckkrn # No args, put all files on coverage

lck:
	$(S) echo $(patsubst %,val_%,$(CHECKS))

include $(topdir)/var/make/check.mk

ifeq ($(NOCOV),)
CKLFLGS += --coverage -fprofile-arcs -ftest-coverage
endif
CKLFLGS += -lpthread
ifeq ($(USE_ATOMIC),y)
CKLFLGS += -latomic
endif

TEST_SRC += $(wildcard $(srcdir)/basic/*.c)
TEST_SRC += $(srcdir)/tests/stub.c
# SYNC_SRC += $(srcdir)/core/futex.c
# SYNC_SRC += $(srcdir)/stdc/cnd.c
# SYNC_SRC += $(srcdir)/stdc/mtx.c
SYNC_SRC += $(srcdir)/_$(target_os)/src/thrd.c
SYNC_SRC += $(srcdir)/tests/sched.c


# cksync_src-y += $(TEST_SRC) $(SYNC_SRC)
# cksync_src-y += $(srcdir)/tests/tst_sync.c
# $(eval $(call link_bin,cksync,cksync_src,CKLFLGS))

ckutils_src-y += $(TEST_SRC)
ckutils_src-y += $(srcdir)/tests/tst_utils.c
$(eval $(call link_bin,ckutils,ckutils_src,CKLFLGS))

ckpipe_src-y += $(TEST_SRC) $(SYNC_SRC)
ckpipe_src-y += $(srcdir)/files/pipe.c
ckpipe_src-y += $(srcdir)/tests/tst_pipe.c
$(eval $(call link_bin,ckpipe,ckpipe_src,CKLFLGS))

ckblk_src-y += $(TEST_SRC) $(SYNC_SRC)
ckblk_src-y += $(srcdir)/files/blk.c
ckblk_src-y += $(srcdir)/tests/tst_blk.c
$(eval $(call link_bin,ckblk,ckblk_src,CKLFLGS))

ckgfx_src-y += $(srcdir)/files/gfx.c
ckgfx_src-y += $(srcdir)/tests/stub.c
ckgfx_src-y += $(srcdir)/tests/tst_gfx.c
$(eval $(call link_bin,ckgfx,ckgfx_src,CKLFLGS))

ckwin_src-y += $(TEST_SRC) $(SYNC_SRC)
ckwin_src-y += $(srcdir)/core/debug.c
ckwin_src-y += $(srcdir)/core/timer.c
ckwin_src-y += $(srcdir)/files/pipe.c
ckwin_src-y += $(srcdir)/files/wmgr.c
ckwin_src-y += $(srcdir)/files/gfx.c
ckwin_src-y += $(srcdir)/tests/stub.c
ckwin_src-y += $(srcdir)/tests/tst_win.c
$(eval $(call link_bin,ckwin,ckwin_src,CKLFLGS))

cknet_src-y += $(TEST_SRC) $(SYNC_SRC)
cknet_src-y += $(srcdir)/core/debug.c
cknet_src-y += $(srcdir)/files/pipe.c
cknet_src-y += $(srcdir)/net/net.c
cknet_src-y += $(srcdir)/net/socket.c
cknet_src-y += $(srcdir)/net/local.c
cknet_src-y += $(srcdir)/tests/tst_net.c
$(eval $(call link_bin,cknet,cknet_src,CKLFLGS))

# ckip4_src-y += $(TEST_SRC) $(SYNC_SRC)
# ckip4_src-y += $(srcdir)/core/debug.c
# ckip4_src-y += $(srcdir)/files/pipe.c
# ckip4_src-y += $(srcdir)/net/net.c
# ckip4_src-y += $(srcdir)/net/socket.c
# ckip4_src-y += $(srcdir)/net/local.c
ckip4_src-y += $(srcdir)/tests/tst_ip4.c
$(eval $(call link_bin,ckip4,ckip4_src,CKLFLGS))

ckelf_src-y += $(TEST_SRC)
ckelf_src-y += $(srcdir)/core/elf.c
# ckelf_src-y += $(srcdir)/core/dlib.c
ckelf_src-y += $(srcdir)/tests/tst_elf.c
$(eval $(call link_bin,ckelf,ckelf_src,CKLFLGS))

ckmem_src-y += $(TEST_SRC)
ckmem_src-y += $(wildcard $(srcdir)/mem/*.c)
ckmem_src-y += $(srcdir)/core/debug.c
ckmem_src-y += $(srcdir)/tests/tst_mem.c
ckmem_src-y += $(topdir)/arch/_simu/mmu.c
$(eval $(call link_bin,ckmem,ckmem_src,CKLFLGS))

ckvfs_src-y += $(TEST_SRC) $(SYNC_SRC)
ckvfs_src-y += $(wildcard $(srcdir)/vfs/*.c)
ckvfs_src-y += $(srcdir)/core/debug.c
ckvfs_src-y += $(srcdir)/tests/tst_vfs.c
ckvfs_src-y += $(srcdir)/tests/timer.c
ckvfs_src-y += $(topdir)/arch/_simu/hostfs.c
$(eval $(call link_bin,ckvfs,ckvfs_src,CKLFLGS))

ckkrn_src-y += $(wildcard $(srcdir)/basic/*.c)
ckkrn_src-y += $(wildcard $(srcdir)/core/*.c)
ckkrn_src-y += $(wildcard $(srcdir)/files/*.c)
ckkrn_src-y += $(wildcard $(srcdir)/mem/*.c)
ckkrn_src-y += $(wildcard $(srcdir)/net/*.c)
# ckkrn_src-y += $(wildcard $(srcdir)/net/ip4/*.c)
ckkrn_src-y += $(wildcard $(srcdir)/task/*.c)
ckkrn_src-y += $(wildcard $(srcdir)/vfs/*.c)
ckkrn_src-y += $(wildcard $(topdir)/arch/_simu/*.c)
# ckkrn_src-y += $(srcdir)/stdc/mtx.c
# ckkrn_src-y += $(srcdir)/stdc/cnd.c
ckkrn_src-y += $(srcdir)/_$(target_os)/src/thrd.c
$(eval $(call link_bin,ckkrn,ckkrn_src,CKLFLGS))



CLILFLGS = $(CKLFLGS) -ldl -rdynamic
cli_vfs_src-y += $(wildcard $(srcdir)/basic/*.c)
cli_vfs_src-y += $(srcdir)/core/debug.c $(srcdir)/core/bio.c $(srcdir)/core/timer.c
cli_vfs_src-y += $(srcdir)/files/pipe.c $(srcdir)/files/blk.c $(srcdir)/files/gfx.c
cli_vfs_src-y += $(wildcard $(srcdir)/vfs/*.c)
cli_vfs_src-y += $(wildcard $(srcdir)/_$(target_os)/src/*.c)
cli_vfs_src-y += $(srcdir)/tests/stub.c
cli_vfs_src-y += $(srcdir)/tests/imgdk.c
cli_vfs_src-y += $(srcdir)/tests/sched.c
cli_vfs_src-y += $(srcdir)/tests/cli_vfs.c

$(eval $(call link_bin,cli_vfs,cli_vfs_src,CLILFLGS))


ifeq ($(NODEPS),)
-include $(call fn_deps,SRCS-y)
endif
