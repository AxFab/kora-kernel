#      This file is part of the KoraOS project.
#  Copyright (C) 2015-2021  <Fabien Bavent>
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
topdir ?= $(shell readlink -f $(dir $(word 1,$(MAKEFILE_LIST))))
gendir ?= $(shell pwd)

include $(topdir)/make/global.mk
ASM_EXT := asm
srcdir = $(topdir)/src
arcdir = $(topdir)/arch/$(target_arch)
kname = kora-$(target_arch).krn

all: kernel drivers libs bins

kernel: $(bindir)/$(kname)

initrd: $(prefix)/boot/bootrd.tar

include $(topdir)/make/build.mk
include $(topdir)/make/drivers.mk

# -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
# Setup compile flags
CFLAGS ?= -Wall -Wextra -Wno-unused-parameter -ggdb
CFLAGS_inc  = -I$(topdir)/include
CFLAGS_inc += -I$(topdir)/src/_$(target_os)/include
CFLAGS_inc += -I$(topdir)/arch/$(target_arch)/include
CFLAGS_def  = -D_DATE_=\"'$(DATE)'\" -D_OSNAME_=\"'$(LINUX)'\"
CFLAGS_def += -D_GITH_=\"'$(GIT)'\" -D_VTAG_=\"'$(VERSION)'\"


# -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
# Rule to build the kernel
CFLAGS_kr += $(CFLAGS)
CFLAGS_kr += -ffreestanding $(CFLAGS_inc) $(CFLAGS_def)

ifeq ($(target_os),kora)
CFLAGS_kr += -DKORA_KRN -D__NO_SYSCALL
else
CFLAGS_kr += -lpthread
ifeq ($(NOCOV),)
CFLAGS_kr += --coverage -fprofile-arcs -ftest-coverage
endif
ifeq ($(USE_ATOMIC),y)
CFLAGS_kr += -latomic
endif
endif


SRCS_kr += $(wildcard $(srcdir)/stdc/*.c)
SRCS_kr += $(wildcard $(srcdir)/vfs/*.c)
SRCS_kr += $(wildcard $(srcdir)/mem/*.c)
SRCS_kr += $(wildcard $(srcdir)/tasks/*.c)
SRCS_kr += $(wildcard $(srcdir)/core/*.c)
SRCS_kr += $(wildcard $(srcdir)/net/*.c)
# SRCS_kr += $(wildcard $(srcdir)/snd/*.c)
# SRCS_kr += $(wildcard $(srcdir)/net/ip4/*.c)
SRCS_kr += $(wildcard $(arcdir)/*.$(ASM_EXT))
SRCS_kr += $(wildcard $(arcdir)/*.c)

include $(topdir)/arch/$(target_arch)/make.mk

$(eval $(call comp_source,kr,CFLAGS_kr))
$(bindir)/$(kname): $(call fn_objs,SRCS_kr,kr)
	$(S) mkdir -p $(dir $@)
	$(Q) echo "    LD  "$@
	$(V) $(CC) -T $(arcdir)/kernel.ld -o $@ $^ -nostdlib -lgcc

# -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
# Build drivers

CFLAGS_dr += $(CFLAGS)
CFLAGS_dr += -ffreestanding -fPIC $(CFLAGS_inc) $(CFLAGS_def)

LFLAGS_dr += -lc

# DRV = vfat ext2 isofs
DRV  = fs/vfat fs/isofs fs/ext2
DRV += pc/ata pc/e1000 pc/ps2 pc/vga
DRV += misc/vbox
# DRV += pc/ac97 pc/sb16

include $(foreach dir,$(DRV),$(topdir)/drivers/$(dir)/Makefile)

# -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
# Build tests
CHECKS += ckstdc ckvfs ckmem cktask cknet

lck:
	$(S) echo $(patsubst %,val_%,$(CHECKS))

include $(topdir)/make/check.mk


SRC_STDC += $(wildcard $(srcdir)/_$(target_os)/*.c)
SRC_STDC += $(srcdir)/stdc/bbtree.c
SRC_STDC += $(srcdir)/stdc/debug.c
SRC_STDC += $(srcdir)/stdc/hmap.c
SRC_STDC += $(srcdir)/stdc/sem.c

SRC_ckvfs = $(SRC_STDC) $(srcdir)/tests/ckvfs.c
SRC_ckvfs += $(wildcard $(srcdir)/vfs/*.c)
# $(eval $(call link_bin,ckvfs,SRC_ckvfs,CFLAGS))



# -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
# Build cli-*

CFLAGS_cli += $(CFLAGS) -fPIC $(CFLAGS_inc) $(CFLAGS_def)
LFLAGS_cli += -lpthread

SRC_kcore += $(topdir)/src/stdc/bbtree.c
SRC_kcore += $(topdir)/src/stdc/debug.c
SRC_kcore += $(topdir)/src/stdc/hmap.c
SRC_kcore += $(topdir)/src/stdc/sem.c
SRC_kcore += $(wildcard $(topdir)/cli/_unix/*.c)

# SRC_kcore += $(topdir)/src/tests/stub.c

SRC_clivfs += $(wildcard $(topdir)/src/vfs/*.c)
SRC_clivfs += $(wildcard $(topdir)/cli/vfs/*.c)
SRC_clivfs += $(SRC_kcore)


$(eval $(call comp_source,cli,CFLAGS_cli))
$(eval $(call link_bin,cli-vfs,SRC_clivfs,LFLAGS_cli,cli))


# -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
# -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

$(prefix)/boot/$(kname): $(bindir)/$(kname)
	$(S) mkdir -p $(dir $@)
	$(Q) echo "    INSTALL "$@
	$(V) $(INSTALL) $< $@

install: $(prefix)/boot/$(kname) initrd $(INSTALL_BINS)

drivers: $(DRVS)

libs: $(LIBS)

bins: $(BINS)

$(prefix)/boot/bootrd.tar: $(INSTALL_DRVS)
	$(Q) echo "    TAR "$@
	$(V) cd $(prefix)/boot/mods && tar cf $@ $(patsubst $(prefix)/boot/mods/%,%,$^)

# -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

ifeq ($(NODEPS),)
-include $(call fn_deps,SRCS_kr,kr)
-include $(call fn_deps,SRCS_dr,dr)
endif
