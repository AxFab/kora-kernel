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

include $(topdir)/var/make/global.mk
# srcdir = $(topdir)/src

all: libname
install: $(prefix)/lib/libname.so

# -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

CFLAGS ?= -Wall -Wextra -Wno-unused-parameter -ggdb
CFLAGS += -I$(topdir)/include -fPIC

# SRCS-y += $(wildcard $(srcdir)/dir/*.c)
# SRCS-y += $(wildcard $(srcdir)/$(DISTO)/*.c)

include $(topdir)/var/make/build.mk
# include $(topdir)/arch/$(target_arch)/make.mk

# -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

# DISTO ?= my-disto

ifeq ($(DISTO),A)
LFLAGS += -lA
else ifeq ($(DISTO),B)
LFLAGS += -lB
else ifeq ($(DISTO),C)
CFLAGS += -Dname=C
LFLAGS += -lC
endif

# -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

$(eval $(call link_shared,name,SRCS,LFLAGS))

# -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

ifeq ($(NODEPS),)
include $(call fn_deps, SRCS)
endif
