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
topdir ?= $(shell readlink -f $(dir $(word 1,$(MAKEFILE_LIST))))
gendir ?= $(shell pwd)

include $(topdir)/make/global.mk
ASM_EXT := asm
srcdir = $(topdir)/src
arcdir = $(topdir)/arch/$(target_arch)
kname = kora-$(target_arch).krn

all: kernel

kernel: $(bindir)/$(kname)

install: $(prefix)/boot/$(kname)

$(prefix)/boot/$(kname): $(bindir)/$(kname)
	$(S) mkdir -p $(dir $@)
	$(Q) echo "    INSTALL "$@
	$(V) $(INSTALL) $< $@

# -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
# Setup compile flags

CFLAGS ?= -Wall -Wextra -Wno-unused-parameter -ggdb
CFLAGS += -ffreestanding
CFLAGS += -I$(topdir)/include
CFLAGS += -I$(topdir)/src/_$(target_os)/include
CFLAGS += -I$(topdir)/arch/$(target_arch)/include
CFLAGS += -D_DATE_=\"'$(DATE)'\" -D_OSNAME_=\"'$(LINUX)'\"
CFLAGS += -D_GITH_=\"'$(GIT)'\" -D_VTAG_=\"'$(VERSION)'\"

ifeq ($(target_os),kora)
# Build the kernel
CFLAGS += -DKORA_KRN -D__NO_SYSCALL

else
# Build hosted unit-tests
CFLAGS += -lpthread
ifeq ($(NOCOV),)
CFLAGS += --coverage -fprofile-arcs -ftest-coverage
endif
ifeq ($(USE_ATOMIC),y)
CFLAGS += -latomic
endif

endif

include $(topdir)/make/build.mk

# -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
# Rule to build the kernel
SRCS += $(wildcard $(srcdir)/stdc/*.c)
SRCS += $(wildcard $(srcdir)/vfs/*.c)
SRCS += $(wildcard $(srcdir)/mem/*.c)
SRCS += $(wildcard $(srcdir)/tasks/*.c)
SRCS += $(wildcard $(srcdir)/core/*.c)
# SRCS += $(wildcard $(srcdir)/net/*.c)
# SRCS += $(wildcard $(srcdir)/net/ip4/*.c)
SRCS += $(wildcard $(arcdir)/*.$(ASM_EXT))
SRCS += $(wildcard $(arcdir)/*.c)

include $(topdir)/arch/$(target_arch)/make.mk

$(bindir)/$(kname): $(call fn_objs,SRCS)
	$(S) mkdir -p $(dir $@)
	$(Q) echo "    LD  "$@
	$(V) $(CC) -T $(arcdir)/kernel.ld -o $@ $^ -nostdlib -lgcc


# -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

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
$(eval $(call link_bin,ckvfs,SRC_ckvfs,CFLAGS))



ifeq ($(NODEPS),)
-include $(call fn_deps,SRCS)
endif
