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
CFLAGS += -Wno-multichar -Wno-implicit-fallthrough
CFLAGS += -D_DATE_=\"'$(DATE)'\" -D_OSNAME_=\"'$(LINUX)'\"
CFLAGS += -D_GITH_=\"'$(GIT)'\" -D_VTAG_=\"'$(VERSION)'\"
CFLAGS += -ggdb3 -I$(topdir)/include 

COV_FLAGS += --coverage -fprofile-arcs -ftest-coverage
KRN_FLAGS += -DKORA_STDC
# KRN_FLAGS += -DSPLOCK_TICKET

include $(srcdir)/drivers/drivers.mk
include $(topdir)/arch/$(target_arch)/make.mk

# -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
# We define modes of compiling
chk_CFLAGS += $(CFLAGS)  $(COV_FLAGS) -D_FAKE_TIME -D_FAKE_TASK
chk_CFLAGS += -I$(topdir)/src/tests/include 
chk_CFLAGS += -I$(topdir)/src/tests/_${CC}/include-${target_arch} 
krn_CFLAGS += $(CFLAGS)  $(KRN_FLAGS)
krn_CFLAGS += -I$(topdir)/arch/$(target_arch)/include 
krn_CFLAGS += -I$(topdir)/include/cc
$(eval $(call ccpl,chk))
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
kImage_src-y += $(wildcard $(topdir)/arch/$(target_arch)/src/*.asm)
kImage_src-y += $(wildcard $(topdir)/arch/$(target_arch)/src/*.c)
kImage_src-y += $(wildcard $(srcdir)/libc/*.c)
# kImage_src-y += $(wildcard $(srcdir)/scall/*.c)
kImage_src-y += $(core_src-y)
kImage_src-y += $(drv_src-y)
# kImage_omit-y += $(srcdir)/io/seat.c $(srcdir)/io/termio.c
$(eval $(call kimg,kImage,krn))
DV_UTILS += $(bindir)/kImage



# -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
# T E S T I N G   U T I L I T I E S -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

ckFs_src-y += $(wildcard $(srcdir)/libc/*.c)
ckFs_src-y += $(wildcard $(srcdir)/vfs/*.c)
ckFs_src-y += $(wildcard $(srcdir)/core/bio.c)
ckFs_src-y += $(wildcard $(srcdir)/core/debug.c)
ckFs_src-y += $(wildcard $(srcdir)/files/ioblk.c)
ckFs_src-y += $(wildcard $(srcdir)/task/async.c)
ckFs_src-y += $(wildcard $(srcdir)/drivers/fs/fat/*.c)
ckFs_src-y += $(wildcard $(srcdir)/drivers/fs/isofs/*.c)
ckFs_src-y += $(wildcard $(srcdir)/drivers/disk/imgdk/*.c)
ckFs_src-y += $(wildcard $(srcdir)/tests/fs/*.c)
ckFs_src-y += $(srcdir)/tests/_stub/core.c  
ckFs_src-y += $(srcdir)/tests/_stub/irq.c
ckFs_src-y += $(srcdir)/tests/_stub/mem.c
ckFs_omit-y += $(srcdir)/libc/mutex.c
ckFs_LFLAGS += $(LFLAGS) $(COV_FLAGS)
$(eval $(call link,ckFs,chk))
# DV_CHECK += $(bindir)/ckFs

# -------------------------

ckFile_src-y += $(wildcard $(srcdir)/files/*.c)
ckFile_omit-y += $(srcdir)/files/wmgr.c
ckFile_src-y += $(srcdir)/arch/um2/common.c $(srcdir)/arch/um2/irq.c
ckFile_src-y += $(srcdir)/core/debug.c  $(srcdir)/arch/um2/cpu.c
ckFile_src-y += $(srcdir)/tests/ck_file.c
ckFile_LFLAGS += $(LFLAGS) $(COV_FLAGS)
$(eval $(call link,ckFile,chk))
# DV_CHECK += $(bindir)/ckFile

# -------------------------

ckMem_src-y += $(wildcard $(srcdir)/mem/*.c)
ckMem_src-y += $(srcdir)/libc/bbtree.c
ckMem_src-y += $(srcdir)/libc/hmap.c
ckMem_src-y += $(srcdir)/core/debug.c
ckMem_src-y += $(wildcard $(srcdir)/task/async.c)
ckMem_src-y += $(wildcard $(srcdir)/tests/mem/*.c)
ckMem_src-y += $(srcdir)/tests/_stub/core.c
ckMem_src-y += $(srcdir)/tests/_stub/irq.c
ckMem_src-y += $(srcdir)/tests/_stub/mmu.c
ckMem_src-y += $(srcdir)/tests/_stub/vfs.c
ckMem_LFLAGS += $(LFLAGS) $(COV_FLAGS)
$(eval $(call link,ckMem,chk))
DV_CHECK += $(bindir)/ckMem

# -------------------------

ckNet_src-y += $(wildcard $(srcdir)/net/*.c)
ckNet_src-y += $(wildcard $(srcdir)/arch/um2/*.c)
ckNet_src-y += $(srcdir)/core/debug.c $(srcdir)/libc/random.c $(srcdir)/libc/hmap.c
ckNet_src-y += $(wildcard $(srcdir)/tests/net/*.c)
ckNet_src-y += $(srcdir)/tests/_stub/core.c
ckNet_src-y += $(srcdir)/tests/_stub/irq.c
ckNet_src-y += $(srcdir)/tests/_stub/vfs.c
ckNet_LFLAGS += $(LFLAGS) $(COV_FLAGS)
ckNet_LIBS += -lpthread
$(eval $(call link,ckNet,chk))
DV_CHECK += $(bindir)/ckNet

# -------------------------

ckTask_src-y += $(wildcard $(srcdir)/task/*.c)
ckTask_src-y += $(srcdir)/libc/bbtree.c $(srcdir)/libc/setjmp_$(target_arch).asm
ckTask_src-y += $(srcdir)/arch/um2/common.c $(srcdir)/arch/um2/irq.c
ckTask_src-y += $(srcdir)/core/debug.c $(srcdir)/arch/um2/cpu.c
ckTask_src-y += $(srcdir)/tests/ck_task.c
ckTask_LFLAGS += $(LFLAGS) $(COV_FLAGS)
$(eval $(call link,ckTask,chk))
# DV_CHECK += $(bindir)/ckTask

# -------------------------

ckUtils_src-y += $(wildcard $(srcdir)/libc/*.c)
ckUtils_src-y += $(wildcard $(srcdir)/tests/utils/*.c)
ckUtils_src-y += $(srcdir)/tests/_stub/core.c  $(srcdir)/tests/_stub/irq.c
ckUtils_src-y += $(srcdir)/tests/_stub/vfs.c
ckUtils_omit-y += $(srcdir)/libc/mutex.c
ckUtils_LFLAGS += $(LFLAGS) $(COV_FLAGS)
$(eval $(call link,ckUtils,chk))
DV_CHECK += $(bindir)/ckUtils

# -------------------------

ckVfs_src-y += $(wildcard $(srcdir)/vfs/*.c)
ckVfs_src-y += $(srcdir)/libc/bbtree.c $(srcdir)/libc/hmap.c
ckVfs_src-y += $(drv_src-y)
ckVfs_src-y += $(srcdir)/arch/um2/common.c $(srcdir)/arch/um2/irq.c
ckVfs_src-y += $(srcdir)/core/debug.c
ckVfs_src-y += $(srcdir)/tests/ck_vfs.c
ckVfs_LFLAGS += $(LFLAGS) $(COV_FLAGS)
$(eval $(call link,ckVfs,chk))
# DV_CHECK += $(bindir)/ckVfs














