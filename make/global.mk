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
#  This makefile is generic.

# D I R E C T O R I E S -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
prefix ?= /usr/local
topdir ?= $(shell readlink -f $(dir $(word 1,$(MAKEFILE_LIST))))
gendir ?= $(shell pwd)
srcdir := $(topdir)
outdir := $(gendir)/obj
bindir := $(gendir)/bin
libdir := $(gendir)/lib

# T A R G E T -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
host ?= $(shell $(topdir)/make/host.sh)
host_arch := $(word 1,$(subst -, ,$(host)))
host_vendor := $(word 2,$(subst -, ,$(host)))
host_os := $(patsubst $(host_arch)-$(host_vendor)-%,%,$(host))

target ?= $(host)
target_arch := $(word 1,$(subst -, ,$(target)))
target_vendor := $(word 2,$(subst -, ,$(target)))
target_os := $(patsubst $(target_arch)-$(target_vendor)-%,%,$(target))

S := @
V := $(shell [ -z $(VERBOSE) ] && echo @)
Q := $(shell [ -z $(QUIET) ] && echo @ || echo @true)

# C O M M A N D S -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
AS ?= $(CROSS)as
AR ?= $(CROSS)ar
CC ?= $(CROSS)gcc
CXX ?= $(CROSS)g++
LD ?= $(CROSS)ld
LDC ?= $(CC)
LDCX ?= $(CXX)
NM ?= nm
INSTALL ?= install
PKC ?= pkg-config

ASM_EXT := s

DATE := $(shell date '+%Y-%m-%d')
GIT_H := $(shell git --git-dir=$(topdir)/.git rev-parse --short HEAD 2> /dev/null)$(shell if [ -n "$(git --git-dir=$(topdir)/.git status -suno)"]; then echo '+'; fi)
GIT_V := $(shell git --git-dir=$(topdir)/.git describe 2> /dev/null)
VERSION ?= $(GIT_V:v=)

# A V O I D   D E P E N D E N C Y -=-=-=-=-=-=-=-=-=-=-=-
ifeq ($(shell [ -d $(outdir) ] || echo N ),N)
NODEPS ?= 1
endif
ifeq ($(MAKECMDGOALS),help)
NODEPS ?= 1
endif
ifeq ($(MAKECMDGOALS),clean)
NODEPS ?= 1
endif
ifeq ($(MAKECMDGOALS),distclean)
NODEPS ?= 1
endif


define fn_objs
	$(patsubst $(topdir)/%.c,$(outdir)/%.o,$(patsubst $(topdir)/%.$(ASM_EXT),$(outdir)/%.o,$($(1))))
endef
define fn_deps
	$(patsubst $(topdir)/%.c,$(outdir)/%.d,$(patsubst $(topdir)/%.$(ASM_EXT),,$($(1))))
endef
define fn_inst
	$(patsubst $(gendir)/%,$(prefix)/%,$(1))
endef
define fn_flcp
	$(patsubst $(topdir)/%,$(prefix)/%,$(1))
endef
