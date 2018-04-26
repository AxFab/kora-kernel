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
VERSION=1.0-$(GIT)

CFLAGS += -Wall -Wextra -Wno-unused-parameter -fno-builtin -Wno-implicit-fallthrough
CFLAGS += -D_DATE_=\"'$(DATE)'\" -D_OSNAME_=\"'$(LINUX)'\"
CFLAGS += -D_GITH_=\"'$(GIT)'\" -D_VTAG_=\"'$(VERSION)'\"
CFLAGS += -Wno-multichar
# CFLAGS += -nostdinc
# ifneq ($(target_os),smkos)

COV_FLAGS += --coverage -fprofile-arcs -ftest-coverage
# # CFLAGS += -DNDEBUG
# LFLAGS += --coverage -fprofile-arcs -ftest-coverage
# endif

CFLAGS += -ggdb3 -I$(topdir)/include -I$(topdir)/include/cc
# CFLAGS += -nostdinc
# -I../libc/include
# CFLAGS += `pkg-config --cflags cairo x11`

# We define one mode of compiling `std`
std_CFLAGS := $(CFLAGS) -I$(topdir)/include/arch/um $(COV_FLAGS)
krn_CFLAGS := $(CFLAGS) -I$(topdir)/include/arch/x86 -DKORA_STDC
mod_CFLAGS := $(CFLAGS) -I$(topdir)/include/arch/x86 -DKORA_STDC -DK_MODULE
$(eval $(call ccpl,std))
$(eval $(call ccpl,krn))
$(eval $(call ccpl,mod))

# We create the `tests` delivery
tests_src-y += $(wildcard $(srcdir)/skc/*.c)
tests_src-y += $(wildcard $(srcdir)/libc/*.c)
tests_src-y += $(wildcard $(srcdir)/core/*.c)
tests_src-y += $(wildcard $(srcdir)/tests/*.c)
tests_src-y += $(wildcard $(srcdir)/arch/um/*.c)
tests_src-y += $(wildcard $(srcdir)/arch/um/*.asm)
tests_src-y += $(wildcard $(srcdir)/fs/tmpfs/*.c)
tests_src-y += $(wildcard $(srcdir)/fs/isofs/*.c)
tests_src-y += $(wildcard $(srcdir)/drv/img/*.c)
tests_src-y += $(wildcard $(srcdir)/drv/mbr/*.c)
tests_omit-y += $(srcdir)/libc/string.c $(srcdir)/core/common.c $(srcdir)/core/launch.c $(srcdir)/core/scall.c
tests_omit-y += $(srcdir)/core/vfs.c $(srcdir)/tests/vfs.c $(srcdir)/core/net.c
# tests_omit-y += $(srcdir)/libc/format_vfprintf.c $(srcdir)/libc/format_print.c $(srcdir)/tests/format.c
tests_LFLAGS := $(LFLAGS)
tests_LIBS := $(shell pkg-config --libs check)
# $(eval $(call link,tests,std))
# DV_UTILS += $(bindir)/tests

# -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
# We create the `kernel` delivery
# kSim_src-y += $(wildcard $(srcdir)/arch/um/*.asm)
kSim_src-y += $(wildcard $(srcdir)/arch/um2/*.c)
kSim_src-y += $(wildcard $(srcdir)/skc/*.c)
kSim_src-y += $(wildcard $(srcdir)/libc/*.c)
kSim_src-y += $(wildcard $(srcdir)/core/*.c)
kSim_src-y += $(wildcard $(srcdir)/core/io/*.c)
kSim_src-y += $(wildcard $(srcdir)/core/tsk/*.c)
kSim_src-y += $(wildcard $(srcdir)/core/mem/*.c)
kSim_src-y += $(wildcard $(srcdir)/core/vfs/*.c)
kSim_src-y += $(wildcard $(srcdir)/scall/*.c)
# kSim_src-y += $(wildcard $(srcdir)/drv/pci/*.c)
kSim_src-y += $(wildcard $(srcdir)/drv/img/*.c)
# kSim_src-y += $(wildcard $(srcdir)/drv/ps2/*.c)
# kImg_src-y += $(wildcard $(srcdir)/drv/vga/*.c)
# kImg_src-y += $(wildcard $(srcdir)/drv/e1000/*.c)
# kImg_src-y += $(wildcard $(srcdir)/drv/am79C973/*.c)
# kSim_src-y += $(wildcard $(srcdir)/fs/tmpfs/*.c)
# kSim_src-y += $(wildcard $(srcdir)/fs/devfs/*.c)
kSim_src-y += $(wildcard $(srcdir)/fs/isofs/*.c)
kSim_omit-y += $(srcdir)/core/common.c
kSim_omit-y += $(srcdir)/core/io/seat.c $(srcdir)/core/io/termio.c
kSim_omit-y += $(srcdir)/libc/format_vfprintf.c $(srcdir)/libc/format_print.c
kSim_omit-y += $(srcdir)/libc/format_vfscanf.c $(srcdir)/libc/format_scan.c
# kSim_LFLAGS := $(LFLAGS) `pkg-config --libs cairo x11`
$(eval $(call link,kSim,std))
DV_UTILS += $(bindir)/kSim



# -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
# We create the `kernel` delivery
kImg_src-y += $(wildcard $(srcdir)/arch/x86/*.asm)
kImg_src-y += $(wildcard $(srcdir)/arch/x86/*.c)
kImg_src-y += $(wildcard $(srcdir)/skc/*.c)
kImg_src-y += $(wildcard $(srcdir)/libc/*.c)
kImg_src-y += $(wildcard $(srcdir)/core/*.c)
kImg_src-y += $(wildcard $(srcdir)/core/io/*.c)
kImg_src-y += $(wildcard $(srcdir)/core/tsk/*.c)
kImg_src-y += $(wildcard $(srcdir)/core/mem/*.c)
kImg_src-y += $(wildcard $(srcdir)/core/vfs/*.c)
kImg_src-y += $(wildcard $(srcdir)/scall/*.c)
kImg_src-y += $(wildcard $(srcdir)/drv/pci/*.c)
kImg_src-y += $(wildcard $(srcdir)/drv/ata/*.c)
kImg_src-y += $(wildcard $(srcdir)/drv/ps2/*.c)
# kImg_src-y += $(wildcard $(srcdir)/drv/vga/*.c)
# kImg_src-y += $(wildcard $(srcdir)/drv/e1000/*.c)
# kImg_src-y += $(wildcard $(srcdir)/drv/am79C973/*.c)
# kImg_src-y += $(wildcard $(srcdir)/fs/tmpfs/*.c)
# kImg_src-y += $(wildcard $(srcdir)/fs/devfs/*.c)
kImg_src-y += $(wildcard $(srcdir)/fs/isofs/*.c)
kImg_omit-y += $(srcdir)/core/io/seat.c $(srcdir)/core/io/termio.c

# kImg_LFLAGS := $(LFLAGS)
$(eval $(call kimg,kImg,krn))
DV_UTILS += $(bindir)/kImg


# Drivers -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
kmod_pci_src-y += $(wildcard $(srcdir)/drv/pci/*.c)
$(eval $(call llib,kmod_pci,mod))
DV_LIBS += $(bindir)/kmod_pci

kmod_ata_src-y += $(wildcard $(srcdir)/drv/ata/*.c)
$(eval $(call llib,kmod_ata,mod))
DV_LIBS += $(bindir)/kmod_ata

kmod_e1000_src-y += $(wildcard $(srcdir)/drv/e1000/*.c)
$(eval $(call llib,kmod_e1000,mod))
DV_LIBS += $(bindir)/kmod_e1000

kmod_iso9660_src-y += $(wildcard $(srcdir)/fs/iso9660/*.c)
$(eval $(call llib,kmod_iso9660,mod))
DV_LIBS += $(bindir)/kmod_iso9660

# T E S T I N G   U T I L I T I E S -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
ckVfs_src-y += $(wildcard $(srcdir)/core/vfs/*.c)
ckVfs_src-y += $(wildcard $(srcdir)/libc/*.c)
ckVfs_src-y += $(wildcard $(srcdir)/drv/img/*.c)
# ckVfs_src-y += $(wildcard $(srcdir)/fs/devfs/*.c)
ckVfs_src-y += $(wildcard $(srcdir)/fs/fat/*.c)
ckVfs_src-y += $(wildcard $(srcdir)/fs/isofs/*.c)
ckVfs_src-y += $(srcdir)/arch/um2/common.c $(srcdir)/arch/um2/irq.c
ckVfs_src-y += $(srcdir)/core/debug.c
ckVfs_src-y += $(srcdir)/tests/ck_vfs.c
ckVfs_omit-y += $(srcdir)/libc/format_vfprintf.c $(srcdir)/libc/format_print.c
ckVfs_LFLAGS += $(LFLAGS) $(COV_FLAGS)
$(eval $(call link,ckVfs,std))
DV_UTILS += $(bindir)/ckVfs

# -------------------------

ckMem_src-y += $(wildcard $(srcdir)/core/mem/*.c)
ckMem_src-y += $(wildcard $(srcdir)/libc/*.c)
# ckMem_src-y += $(wildcard $(srcdir)/drv/img/*.c)
# ckMem_src-y += $(wildcard $(srcdir)/fs/devfs/*.c)
# ckMem_src-y += $(wildcard $(srcdir)/fs/isofs/*.c)
ckMem_src-y += $(srcdir)/arch/um2/common.c $(srcdir)/arch/um2/irq.c
ckMem_src-y += $(srcdir)/core/debug.c $(srcdir)/arch/um2/mmu.c
ckMem_src-y += $(srcdir)/tests/ck_mem.c
ckMem_omit-y += $(srcdir)/libc/format_vfprintf.c $(srcdir)/libc/format_print.c
$(eval $(call link,ckMem,std))
DV_UTILS += $(bindir)/ckMem

# -------------------------

ckFile_src-y += $(wildcard $(srcdir)/core/file/*.c)
ckFile_src-y += $(wildcard $(srcdir)/libc/*.c)

ckFile_src-y += $(srcdir)/arch/um2/common.c $(srcdir)/arch/um2/irq.c
ckFile_src-y += $(srcdir)/core/debug.c $(srcdir)/arch/um2/mmu.c
ckFile_src-y += $(srcdir)/tests/ck_file.c
ckFile_omit-y += $(srcdir)/libc/format_vfprintf.c $(srcdir)/libc/format_print.c
ckFile_LFLAGS += $(LFLAGS) $(COV_FLAGS)
# `pkg-config --libs cairo x11`
$(eval $(call link,ckFile,std))
DV_UTILS += $(bindir)/ckFile



$(outdir)/krn/%.o: $(srcdir)/%.asm
	$(S) mkdir -p $(dir $@)
	$(Q) echo "    ASM "$@
	$(V) nasm -f elf32 -o $@ $^

$(outdir)/std/%.o: $(srcdir)/%.asm
	$(S) mkdir -p $(dir $@)
	$(Q) echo "    ASM "$@
	$(V) nasm -f elf32 -o $@ $^

