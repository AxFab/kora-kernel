#      This file is part of the KoraOS project.
#  Copyright (C) 2015  <Fabien Bavent>
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

include $(srcdir)/drivers/drivers.mk

CFLAGS += -Wall -Wextra -Wno-unused-parameter -fno-builtin
CFLAGS += -D_DATE_=\"'$(DATE)'\" -D_OSNAME_=\"'$(LINUX)'\"
CFLAGS += -D_GITH_=\"'$(GIT)'\" -D_VTAG_=\"'$(VERSION)'\"
CFLAGS += -Wno-multichar -Wno-implicit-fallthrough
CFLAGS += -ggdb3 -I$(topdir)/include -I$(topdir)/include/cc

COV_FLAGS += --coverage -fprofile-arcs -ftest-coverage


# We define one mode of compiling `std`
std_CFLAGS := $(CFLAGS) -I$(topdir)/include/arch/um $(COV_FLAGS)
krn_CFLAGS := $(CFLAGS) -I$(topdir)/include/arch/x86 -DKORA_STDC
mod_CFLAGS := $(CFLAGS) -I$(topdir)/include/arch/x86 -DKORA_STDC -DK_MODULE
$(eval $(call ccpl,std))
$(eval $(call ccpl,krn))
$(eval $(call ccpl,mod))


core_src-y += $(wildcard $(srcdir)/core/*.c)
core_src-y += $(wildcard $(srcdir)/core/files/*.c)
core_src-y += $(wildcard $(srcdir)/core/io/*.c)
core_src-y += $(wildcard $(srcdir)/core/task/*.c)
core_src-y += $(wildcard $(srcdir)/core/mem/*.c)
core_src-y += $(wildcard $(srcdir)/core/vfs/*.c)
core_src-y += $(wildcard $(srcdir)/core/net/*.c)


# -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
# We create the `kernel` delivery
# kSim_src-y += $(wildcard $(srcdir)/arch/um/*.asm)
kSim_src-y += $(wildcard $(srcdir)/arch/um2/*.c)
kSim_src-y += $(wildcard $(srcdir)/libc/*.c)
kSim_src-y += $(wildcard $(srcdir)/scall/*.c)
kSim_src-y += $(drv_src-y) $(core_src-y)
kSim_omit-y += $(srcdir)/core/common.c
kSim_omit-y += $(srcdir)/core/io/seat.c $(srcdir)/core/io/termio.c
kSim_omit-y += $(srcdir)/libc/format_vfprintf.c $(srcdir)/libc/format_print.c
kSim_omit-y += $(srcdir)/libc/format_vfscanf.c $(srcdir)/libc/format_scan.c
$(eval $(call link,kSim,std))
DV_UTILS += $(bindir)/kSim



# -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
# We create the `kernel` delivery
kImg_src-y += $(wildcard $(srcdir)/arch/x86/*.asm)
kImg_src-y += $(wildcard $(srcdir)/arch/x86/*.c)
kImg_src-y += $(wildcard $(srcdir)/libc/*.c)
kImg_src-y += $(wildcard $(srcdir)/scall/*.c)
kImg_src-y += $(drv_src-y) $(core_src-y)
kImg_omit-y += $(srcdir)/core/io/seat.c $(srcdir)/core/io/termio.c
$(eval $(call kimg,kImg,krn))
DV_UTILS += $(bindir)/kImg



# T E S T I N G   U T I L I T I E S -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
ckVfs_src-y += $(wildcard $(srcdir)/core/vfs/*.c)
ckVfs_src-y += $(srcdir)/libc/bbtree.c
ckVfs_src-y += $(drv_src-y)
ckVfs_src-y += $(srcdir)/arch/um2/common.c $(srcdir)/arch/um2/irq.c
ckVfs_src-y += $(srcdir)/core/debug.c
ckVfs_src-y += $(srcdir)/tests/ck_vfs.c
ckVfs_LFLAGS += $(LFLAGS) $(COV_FLAGS)
$(eval $(call link,ckVfs,std))
DV_UTILS += $(bindir)/ckVfs

# -------------------------

ckMem_src-y += $(wildcard $(srcdir)/core/mem/*.c)
ckMem_src-y += $(srcdir)/libc/bbtree.c
ckMem_src-y += $(srcdir)/arch/um2/common.c $(srcdir)/arch/um2/irq.c
ckMem_src-y += $(srcdir)/core/debug.c $(srcdir)/arch/um2/mmu.c
ckMem_src-y += $(srcdir)/tests/ck_mem.c
ckMem_LFLAGS += $(LFLAGS) $(COV_FLAGS)
$(eval $(call link,ckMem,std))
DV_UTILS += $(bindir)/ckMem

# -------------------------

ckFile_src-y += $(wildcard $(srcdir)/core/files/*.c)
ckFile_omit-y += $(srcdir)/core/files/wmgr.c
# ckFile_src-y += $(wildcard $(srcdir)/libc/*.c)
ckFile_src-y += $(srcdir)/arch/um2/common.c $(srcdir)/arch/um2/irq.c
ckFile_src-y += $(srcdir)/core/debug.c  $(srcdir)/arch/um2/cpu.c
# $(srcdir)/arch/um2/mmu.c
ckFile_src-y += $(srcdir)/tests/ck_file.c
ckFile_LFLAGS += $(LFLAGS) $(COV_FLAGS)
$(eval $(call link,ckFile,std))
DV_UTILS += $(bindir)/ckFile

# -------------------------

ckTask_src-y += $(wildcard $(srcdir)/core/task/*.c)
ckTask_src-y += $(srcdir)/libc/bbtree.c $(srcdir)/libc/setjmp_x86_64.asm
ckTask_src-y += $(srcdir)/arch/um2/common.c $(srcdir)/arch/um2/irq.c
ckTask_src-y += $(srcdir)/core/debug.c $(srcdir)/arch/um2/cpu.c
# $(srcdir)/arch/um2/mmu.c
ckTask_src-y += $(srcdir)/tests/ck_task.c
ckTask_LFLAGS += $(LFLAGS) $(COV_FLAGS)
$(eval $(call link,ckTask,std))
DV_UTILS += $(bindir)/ckTask



$(outdir)/krn/%.o: $(srcdir)/%.asm
	$(S) mkdir -p $(dir $@)
	$(Q) echo "    ASM "$@
	$(V) nasm -f elf32 -o $@ $^

$(outdir)/std/%.o: $(srcdir)/%.asm
	$(S) mkdir -p $(dir $@)
	$(Q) echo "    ASM "$@
	$(V) nasm -f elf64 -o $@ $^

