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
CFLAGS += -ggdb3 -I$(topdir)/include -I$(topdir)/include/cc

COV_FLAGS += --coverage -fprofile-arcs -ftest-coverage
KRN_FLAGS += -DKORA_STDC
# KRN_FLAGS += -DSPLOCK_TICKET

include $(srcdir)/drivers/drivers.mk
include $(srcdir)/arch/$(target_arch)/make.mk

# -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
# We define modes of compiling
std_CFLAGS := $(CFLAGS) -I$(topdir)/include/arch/um $(COV_FLAGS)
krn_CFLAGS := $(CFLAGS) -I$(topdir)/include/arch/$(target_arch) $(KRN_FLAGS)
$(eval $(call ccpl,std))
$(eval $(call ccpl,krn))

core_src-y += $(wildcard $(srcdir)/core/*.c)
core_src-y += $(wildcard $(srcdir)/files/*.c)
# core_src-y += $(wildcard $(srcdir)/io/*.c)
core_src-y += $(wildcard $(srcdir)/task/*.c)
core_src-y += $(wildcard $(srcdir)/mem/*.c)
core_src-y += $(wildcard $(srcdir)/vfs/*.c)
core_src-y += $(wildcard $(srcdir)/net/*.c)


# -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
# We create the `kernel` delivery
# kSim_src-y += $(wildcard $(srcdir)/arch/um/*.asm)
kSim_src-y += $(wildcard $(srcdir)/arch/um2/*.c)
kSim_src-y += $(wildcard $(srcdir)/libc/*.c)
kSim_src-y += $(wildcard $(srcdir)/scall/*.c)
kSim_src-y += $(drv_src-y) $(core_src-y)
kSim_omit-y += $(srcdir)/core/common.c
kSim_omit-y += $(srcdir)/core/seat.c $(srcdir)/core/termio.c
kSim_omit-y += $(srcdir)/libc/format_vfprintf.c $(srcdir)/libc/format_print.c
kSim_omit-y += $(srcdir)/libc/format_vfscanf.c $(srcdir)/libc/format_scan.c
$(eval $(call link,kSim,std))
DV_UTILS += $(bindir)/kSim



# -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
# We create the `kernel` delivery
kImage_src-y += $(wildcard $(srcdir)/arch/$(target_arch)/*.asm)
kImage_src-y += $(wildcard $(srcdir)/arch/$(target_arch)/*.c)
kImage_src-y += $(wildcard $(srcdir)/libc/*.c)
kImage_src-y += $(wildcard $(srcdir)/scall/*.c)
kImage_src-y += $(core_src-y)
kImage_src-y += $(drv_src-y)
# kImage_omit-y += $(srcdir)/io/seat.c $(srcdir)/io/termio.c
$(eval $(call kimg,kImage,krn))
DV_UTILS += $(bindir)/kImage



# -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
# T E S T I N G   U T I L I T I E S -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
ckVfs_src-y += $(wildcard $(srcdir)/vfs/*.c)
ckVfs_src-y += $(srcdir)/libc/bbtree.c $(srcdir)/libc/hmap.c
ckVfs_src-y += $(drv_src-y)
ckVfs_src-y += $(srcdir)/arch/um2/common.c $(srcdir)/arch/um2/irq.c
ckVfs_src-y += $(srcdir)/core/debug.c
ckVfs_src-y += $(srcdir)/tests/ck_vfs.c
ckVfs_LFLAGS += $(LFLAGS) $(COV_FLAGS)
$(eval $(call link,ckVfs,std))
# DV_CHECK += $(bindir)/ckVfs

# -------------------------

ckMem_src-y += $(wildcard $(srcdir)/mem/*.c)
ckMem_src-y += $(srcdir)/libc/bbtree.c
ckMem_src-y += $(wildcard $(srcdir)/arch/um2/*.c)
# ckMem_src-y += $(srcdir)/arch/um2/common.c $(srcdir)/arch/um2/irq.c
ckMem_src-y += $(srcdir)/core/debug.c #$(srcdir)/arch/um2/mmu.c
ckMem_src-y += $(srcdir)/tests/ck_mem.c
ckMem_LFLAGS += $(LFLAGS) $(COV_FLAGS)
$(eval $(call link,ckMem,std))
DV_CHECK += $(bindir)/ckMem

# -------------------------

ckFile_src-y += $(wildcard $(srcdir)/files/*.c)
ckFile_omit-y += $(srcdir)/files/wmgr.c
ckFile_src-y += $(srcdir)/arch/um2/common.c $(srcdir)/arch/um2/irq.c
ckFile_src-y += $(srcdir)/core/debug.c  $(srcdir)/arch/um2/cpu.c
# $(srcdir)/arch/um2/mmu.c
ckFile_src-y += $(srcdir)/tests/ck_file.c
ckFile_LFLAGS += $(LFLAGS) $(COV_FLAGS)
$(eval $(call link,ckFile,std))
# DV_CHECK += $(bindir)/ckFile

# -------------------------

ckTask_src-y += $(wildcard $(srcdir)/task/*.c)
ckTask_src-y += $(srcdir)/libc/bbtree.c $(srcdir)/libc/setjmp_$(target_arch).asm
ckTask_src-y += $(srcdir)/arch/um2/common.c $(srcdir)/arch/um2/irq.c
ckTask_src-y += $(srcdir)/core/debug.c $(srcdir)/arch/um2/cpu.c
ckTask_src-y += $(srcdir)/tests/ck_task.c
ckTask_LFLAGS += $(LFLAGS) $(COV_FLAGS)
$(eval $(call link,ckTask,std))
# DV_CHECK += $(bindir)/ckTask

# -------------------------

ckNet_src-y += $(wildcard $(srcdir)/net/*.c)
ckNet_src-y += $(wildcard $(srcdir)/arch/um2/*.c)
ckNet_omit-y = ${srcdir}/arch/um2/mmu.c
# ckNet_src-y += $(srcdir)/arch/um2/common.c $(srcdir)/arch/um2/irq.c
ckNet_src-y += $(srcdir)/core/debug.c $(srcdir)/libc/random.c $(srcdir)/libc/hmap.c
# $(srcdir)/arch/um2/cpu.c
ckNet_src-y += $(srcdir)/tests/ck_net.c
ckNet_LFLAGS += $(LFLAGS) $(COV_FLAGS)
ckNet_LIBS += -lpthread
$(eval $(call link,ckNet,std))
DV_CHECK += $(bindir)/ckNet

# -------------------------

ckUtils_src-y += $(wildcard $(srcdir)/libc/*.c)
ckUtils_src-y += $(srcdir)/arch/um2/common.c $(srcdir)/arch/um2/irq.c
ckUtils_omit-y += $(srcdir)/libc/format_vfprintf.c $(srcdir)/libc/format_print.c
ckUtils_omit-y += $(srcdir)/libc/mutex.c
ckUtils_src-y += $(srcdir)/tests/ck_utils.c
ckUtils_LFLAGS += $(LFLAGS) $(COV_FLAGS)
# ckUtils_LIBS += $(shell pkg-config --libs check)
$(eval $(call link,ckUtils,std))
DV_CHECK += $(bindir)/ckUtils

