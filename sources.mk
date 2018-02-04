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

CFLAGS += -Wall -Wextra -Wno-unused-parameter -fno-builtin
CFLAGS += -D_DATE_=\"'$(DATE)'\" -D_OSNAME_=\"'$(LINUX)'\"
CFLAGS += -D_GITH_=\"'$(GIT)'\" -D_VTAG_=\"'$(VERSION)'\"
CFLAGS += -Wno-multichar
ifneq ($(target_os),smkos)
COV_FLAGS += --coverage -fprofile-arcs -ftest-coverage
# CFLAGS += -DNDEBUG
LFLAGS += --coverage -fprofile-arcs -ftest-coverage
endif

CFLAGS += -ggdb3 -I$(topdir)/include

# We define one mode of compiling `std`
std_CFLAGS := $(CFLAGS) $(COV_FLAGS) -I$(topdir)/include/arch/um
krn_CFLAGS := $(CFLAGS) -I$(topdir)/include/arch/x86 -DKORA_ALLOCATOR
mod_CFLAGS := $(CFLAGS) -I$(topdir)/include/arch/x86 -DKORA_ALLOCATOR -DK_MODULE
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
tests_src-y += $(wildcard $(srcdir)/fs/iso9660/*.c)
tests_src-y += $(wildcard $(srcdir)/drv/img/*.c)
tests_src-y += $(wildcard $(srcdir)/drv/mbr/*.c)
tests_omit-y += $(srcdir)/libc/string.c $(srcdir)/core/common.c $(srcdir)/core/launch.c
tests_omit-y += $(srcdir)/core/vfs.c $(srcdir)/tests/vfs.c $(srcdir)/core/net.c
tests_omit-y += $(srcdir)/libc/format_vfprintf.c $(srcdir)/libc/format_print.c $(srcdir)/tests/format.c
tests_LFLAGS := $(LFLAGS)
tests_LIBS := $(shell pkg-config --libs check)
$(eval $(call link,tests,std))
DV_UTILS += $(bindir)/tests


# We create the `kernel` delivery
kImg_src-y += $(wildcard $(srcdir)/arch/x86/*.asm)
kImg_src-y += $(wildcard $(srcdir)/arch/x86/*.c)
kImg_src-y += $(wildcard $(srcdir)/skc/*.c)
kImg_src-y += $(wildcard $(srcdir)/libc/*.c)
kImg_src-y += $(wildcard $(srcdir)/core/*.c)
kImg_src-y += $(wildcard $(srcdir)/scall/*.c)
kImg_src-y += $(wildcard $(srcdir)/drv/pci/*.c)
kImg_src-y += $(wildcard $(srcdir)/drv/ata/*.c)
kImg_src-y += $(wildcard $(srcdir)/drv/ps2/*.c)
# kImg_src-y += $(wildcard $(srcdir)/drv/vga/*.c)
# kImg_src-y += $(wildcard $(srcdir)/drv/e1000/*.c)
# kImg_src-y += $(wildcard $(srcdir)/drv/am79C973/*.c)
kImg_src-y += $(wildcard $(srcdir)/fs/tmpfs/*.c)
kImg_src-y += $(wildcard $(srcdir)/fs/devfs/*.c)
kImg_src-y += $(wildcard $(srcdir)/fs/iso9660/*.c)
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



$(outdir)/krn/%.o: $(srcdir)/%.asm
	$(S) mkdir -p $(dir $@)
	$(Q) echo "    ASM "$@
	$(V) nasm -f elf32 -o $@ $^

$(outdir)/std/%.o: $(srcdir)/%.asm
	$(S) mkdir -p $(dir $@)
	$(Q) echo "    ASM "$@
	$(V) nasm -f elf32 -o $@ $^

