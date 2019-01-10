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

CFLAGS += -Wall -Wextra -Wno-unused-parameter -Wno-char-subscripts
CFLAGS += -Wno-multichar -Wno-implicit-fallthrough
CFLAGS += -fno-builtin -ffreestanding -fPIC -nostartfiles
CFLAGS += -D_DATE_=\"'$(DATE)'\" -D_OSNAME_=\"'$(LINUX)'\"
CFLAGS += -D_GITH_=\"'$(GIT)'\" -D_VTAG_=\"'$(VERSION)'\"
CFLAGS += -ggdb


COV_FLAGS += --coverage -fprofile-arcs -ftest-coverage
# KRN_FLAGS += -DSPLOCK_TICKET

# include $(srcdir)/drivers/drivers.mk
include $(topdir)/arch/$(target_arch)/make.mk


# -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
# We define compilation modes and associated flags
chk_CFLAGS += $(CFLAGS)  $(COV_FLAGS)
chk_CFLAGS += -I$(topdir)/include
chk_CFLAGS += -I$(topdir)/src/tests/include
chk_CFLAGS += -I$(topdir)/src/tests/_${CC}/include-${target_arch}

krn_CFLAGS += $(CFLAGS)  -DKORA_STDC -DKORA_KRN
krn_CFLAGS += -I$(topdir)/include
krn_CFLAGS += -I$(topdir)/arch/$(target_arch)/include
# krn_CFLAGS += -I$(topdir)/include/cc

std_CFLAGS += $(CFLAGS)  -DKORA_STDC -D__SYS_CALL
std_CFLAGS += -I$(topdir)/include
std_CFLAGS += -I$(topdir)/arch/$(target_arch)/include
# std_CFLAGS += -I$(topdir)/include/cc

$(eval $(call ccpl,chk))
$(eval $(call ccpl,krn))
$(eval $(call ccpl,std))


LFLAGS_app += -nostdlib -L $(libdir) -lc
LFLAGS_ck += --coverage -fprofile-arcs -ftest-coverage

# -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
# K E R N E L   I M A G E -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
kImage_src-y += $(wildcard $(srcdir)/core/*.c)
kImage_src-y += $(wildcard $(srcdir)/files/*.c)
kImage_src-y += $(wildcard $(srcdir)/task/*.c)
kImage_src-y += $(wildcard $(srcdir)/mem/*.c)
kImage_src-y += $(wildcard $(srcdir)/vfs/*.c)
kImage_src-y += $(wildcard $(srcdir)/net/*.c)
kImage_src-y += $(wildcard $(topdir)/arch/$(target_arch)/src/*.asm)
kImage_src-y += $(wildcard $(topdir)/arch/$(target_arch)/src/*.s)
kImage_src-y += $(wildcard $(topdir)/arch/$(target_arch)/src/*.c)
kImage_src-y += $(wildcard $(srcdir)/basic/*.c)
kImage_src-y += $(wildcard $(srcdir)/c89/*.c) $(srcdir)/c11/mutex.c
kImage_src-y += # Drivers
kImage_omit-y += $(srcdir)/c89/libio.c
$(eval $(call kimg,kImage,krn))
DV_UTILS += $(bindir)/kImage

# -------------------------


# -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
# S T A N D A R D   L I B R A I R Y -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
c_src-y += $(wildcard $(topdir)/arch/$(target_arch)/arch/*.c)
c_src-y += $(wildcard $(srcdir)/basic/*.c)
c_src-y += $(wildcard $(srcdir)/c89/*.c)
c_src-y += $(srcdir)/c89.c
c_LFLAGS := $(LFLAGS) -nostdlib
$(eval $(call llib,c,std))
DV_LIBS += $(libdir)/libc.so





# -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
# T E S T I N G   U T I L I T I E S -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
define testo
ck$(1)_src-y += $(wildcard $(srcdir)/basic/*.c)
ck$(1)_src-y += $(wildcard $(srcdir)/c89/*.c)
ck$(1)_src-y += $(wildcard $(srcdir)/tests/$(1)/*.c)
ck$(1)_src-y += $(wildcard $(srcdir)/tests/_stub/*.c)
ck$(1)_omit-y += $(srcdir)/c89/libio.c
ck$(1)_LFLAGS += $(LFLAGS_ck)
DV_CHECK += $(bindir)/ck$(1)
endef
define test
$(eval $(call testo,$1))
$(eval $(call link,ck$(1),chk))
endef

# -------------------------
# ckfiles_src-y += $(wildcard $(srcdir)/files/*.c)
ckfiles_src-y += $(srcdir)/files/pipe.c
ckfiles_src-y += $(srcdir)/task/async.c
ckfiles_omit-y += $(srcdir)/tests/_stub/stub_cpu.c
ckfiles_omit-y += $(srcdir)/tests/_stub/stub_mmu.c
$(eval $(call test,files))
# -------------------------
ckfs_src-y += $(wildcard $(srcdir)/vfs/*.c)
# ckfs_src-y += $(wildcard $(srcdir)/files/*.c) # Yes or no !?
ckfs_src-y += $(srcdir)/files/pipe.c
ckfs_src-y += $(srcdir)/task/async.c
ckfs_src-y += $(srcdir)/core/bio.c
ckfs_src-y += $(srcdir)/core/debug.c
ckfs_src-y += $(wildcard $(srcdir)/drivers/disk/imgdk/*.c)
ckfs_src-y += $(wildcard $(srcdir)/drivers/fs/fat/*.c)
ckfs_src-y += # Drivers
ckfs_omit-y += $(srcdir)/tests/_stub/stub_cpu.c
ckfs_omit-y += $(srcdir)/tests/_stub/stub_mmu.c
ckfs_omit-y += $(srcdir)/tests/_stub/stub_vfs.c
$(eval $(call test,fs))

# -------------------------
ckmem_src-y += $(srcdir)/core/debug.c
ckmem_src-y += $(wildcard $(srcdir)/mem/*.c)
ckmem_omit-y += $(srcdir)/tests/_stub/stub_mem.c
ckmem_omit-y += $(srcdir)/tests/_stub/stub_cpu.c
$(eval $(call test,mem))

# -------------------------
cknet_src-y += $(srcdir)/core/debug.c
cknet_src-y += $(srcdir)/tests/_$(CC)/threads.c
cknet_src-y += $(wildcard $(srcdir)/net/*.c)
cknet_omit-y += $(srcdir)/tests/_stub/stub_mmu.c
cknet_omit-y += $(srcdir)/tests/_stub/stub_cpu.c
cknet_LFLAGS += -lpthread
$(eval $(call test,net))

# -------------------------
ckutils_omit-y += $(srcdir)/tests/_stub/stub_mmu.c
ckutils_omit-y += $(srcdir)/tests/_stub/stub_cpu.c
ckutils_omit-y += $(srcdir)/tests/_stub/stub_irq.c
$(eval $(call test,utils))

# -------------------------



# -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
# T E S T I N G   U T I L I T I E S -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
define utilo
$(1)_src-y += $(topdir)/arch/$(target_arch)/crt0.asm
$(1)_src-y += $(srcdir)/utils/$(1).c
$(1)_LFLAGS := $(LFLAGS_app)
$(1)_DLIBS += c
DV_UTILS += $(bindir)/$(1)
endef
define util
$(eval $(call utilo,$1))
$(eval $(call link,$1,std))
endef

$(eval $(call util,basename))
$(eval $(call util,cat))


