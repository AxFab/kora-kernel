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
#  This makefile is more or less generic.
#  The configuration is on `sources.mk`.
# -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
host ?= $(shell uname -m)-pc-linux-gnu
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

all: libs bins

# D I R E C T O R I E S -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
prefix ?= /usr/local
topdir ?= $(shell readlink -f $(dir $(word 1,$(MAKEFILE_LIST))))
gendir ?= $(shell pwd)
srcdir := $(topdir)/src
outdir := $(gendir)/obj
bindir := $(gendir)/bin
libdir := $(gendir)/lib

# C O M M A N D S -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
CROSS_COMPILE ?= $(CROSS)
AS := $(CROSS_COMPILE)as
AR := $(CROSS_COMPILE)ar
CC := $(CROSS_COMPILE)gcc
CXX := $(CROSS_COMPILE)g++
LD := $(CROSS_COMPILE)ld
NM := $(CROSS_COMPILE)nm

LINUX := $(shell uname -sr)
DATE := $(shell date '+%d %b %Y')
GIT := $(shell git --git-dir=$(topdir)/.git rev-parse --short HEAD)$(shell if [ -n "$(git --git-dir=$(topdir)/.git status -suno)"]; then echo '+'; fi)

# A V O I D   D E P E N D E N C Y -=-=-=-=-=-=-=-=-=-=-=-
ifeq ($(shell [ -d $(outdir) ] || echo N ),N)
NODEPS = 1
endif
ifeq ($(MAKECMDGOALS),help)
NODEPS = 1
endif
ifeq ($(MAKECMDGOALS),clean)
NODEPS = 1
endif
ifeq ($(MAKECMDGOALS),distclean)
NODEPS = 1
endif

# D E L I V E R I E S -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
define obj
	$(patsubst $(srcdir)/%.c,$(outdir)/$(1)/%.$(3),   \
	$(patsubst $(srcdir)/%.cpp,$(outdir)/$(1)/%.$(3), \
	$(patsubst $(srcdir)/%.asm,$(outdir)/$(1)/%.$(3), \
		$(filter-out $($(2)_omit-y),$($(2)_src-y))      \
	)))
endef

define libs
	$(patsubst %,$(libdir)/lib%.$2,$($1))
endef

define llib
DEPS += $(call obj,$2,$1,d)
$1: $(libdir)/lib$1.so
$(libdir)/lib$1.a: $(call obj,$2,$1,o) $(call libs,$1_SLIBS,a)
$(libdir)/lib$1.so: $(call obj,$2,$1,o) $(call libs,$1_SLIBS,a) $(call libs,$1_DLIBS,a)
	$(S) mkdir -p $$(dir $$@)
	$(Q) echo "    LD  "$$@
	$(V) $(CC) -shared $($(1)_LFLAGS) -o $$@ $(call obj,$2,$1,o) $(call libs,$1_SLIBS,a) $($(1)_LIBS)
endef

define link
DEPS += $(call obj,$2,$1,d)
$1: $(bindir)/$1
$(bindir)/$1: $(call obj,$2,$1,o) $(call libs,$1_SLIBS,a) $(call libs,$1_DLIBS,a)
	$(S) mkdir -p $$(dir $$@)
	$(Q) echo "    LD  "$$@
	$(V) $(CC) $($(1)_LFLAGS) -o $$@ $(call obj,$2,$1,o) $(call libs,$1_SLIBS,a) $($(1)_LIBS)
endef

define linkp
DEPS += $(call obj,$2,$1,d)
$1: $(bindir)/$1
$(bindir)/$1: $(call obj,$2,$1,o) $(call libs,$1_SLIBS,a) $(call libs,$1_DLIBS,so) $(patsubst $(srcdir)/%.asm,$(outdir)/%.o,$($1_CRT))
	$(S) mkdir -p $$(dir $$@)
	$(Q) echo "    LD  "$$@
	$(V) $(LD) -T $($(1)_SCP) $($(1)_LFLAGS) -o $$@ -Map $$@.map $(call obj,$2,$1,o) $(call libs,$1_SLIBS,a)
endef

define kimg
DEPS += $(call obj,$2,$1,d)
$1: $(bindir)/$1
$(bindir)/$1: $(call obj,$2,$1,o) # $(outdir)/_$(target_arch)/crtk.o
	$(S) mkdir -p $$(dir $$@)
	$(Q) echo "    LD  "$$@
	$(V) $(LD) -T $(srcdir)/arch/$(target_arch)/kernel.ld $($(1)_LFLAGS) -o $$@ $(call obj,$2,$1,o)
endef

define ccpl
$(outdir)/$(1)/%.o: $(srcdir)/%.c
	$(S) mkdir -p $$(dir $$@)
	$(Q) echo "    CC  "$$@
	$(V) $(CC) -c $($(1)_CFLAGS) -o $$@ $$<
$(outdir)/$(1)/%.d: $(srcdir)/%.c
	$(S) mkdir -p $$(dir $$@)
	$(Q) echo "    CM  "$$@
	$(V) $(CC) -M $($(1)_CFLAGS) -o $$@ $$<
	@ sed "s%.*\.o%$$(@:.d=.o)%" -i $$@
endef

define crt
$2: $(outdir)/$2.o
$(outdir)/$1.o: $(srcdir)/$1.asm
	$(S) mkdir -p $$(dir $$@)
	$(Q) echo "    ASM "$$@
	$(V) nasm -f elf32 -o $$@ $$^
endef

$(libdir)/lib%.a:
	$(S) mkdir -p $(dir $@)
	$(Q) echo "    AR  "$@
	$(V) $(AR) src $@ $^

# S O U R C E S   C O M P I L A T I O N -=-=-=-=-=-=-=-=-
include $(topdir)/sources.mk

# C O M M O N   T A R G E T S -=-=-=-=-=-=-=-=-=-=-=-=-=-
libs: $(DV_LIBS)
bins: $(DV_UTILS)
tests: $(DV_CHECK)
statics: $(patsubst %,$(gendir)/lib/%.a,$(DV_LIBS))
# install: install_dev install_runtime install_utils
# unistall:
# TODO -- rm $(prefix)/XX (without messing with others libs)
clean:
	$(V) rm -rf $(outdir)
distclean: clean
	$(V) rm -rf $(libdir)
	$(V) rm -rf $(bindir)
config:
# TODO -- Create/update configuration headers
check: | $(patsubst $(bindir)/%,val_%, $(DV_CHECK))

# TODO -- Launch unit tests
.PHONY: all libs utils install unistall
.PHONY: clean distclean config check

# P A C K A G I N G -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
release: $(gendir)/$(NAME)-$(target_arch)-$(VERSION).tar.gz

$(gendir)/$(NAME)-$(target_arch)-$(VERSION).tar.gz: $(DV_UTILS) $(DV_LIBS)
	$(Q) echo "  TAR   $@"
	$(V) tar czf $@  -C $(topdir) $(topdir)/include -C $(gendir) $^

# -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

SED_LCOV  = -e '/SF:\/usr.*/,/end_of_record/d'
SED_LCOV += -e '/SF:.*\/src\/tests\/.*/,/end_of_record/d'

# Create coverage HTML report
%.lcov: $(bindir)/%
	@ find -name *.gcda | xargs -r rm
	$(V) CK_FORK=no $<
	$(V) lcov --rc lcov_branch_coverage=1 -c --directory . -b . -o $@ >/dev/null
	@ sed $(SED_LCOV) -i $@

cov_%: %.lcov
	$(V) genhtml --rc lcov_branch_coverage=1 -o $@ $< >/dev/null

val_%: $(bindir)/%
	$(V) valgrind --leak-check=full --show-leak-kinds=all $< 2>&1 #| tee $@

# -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
deps:
	@ echo $(DEPS)

dirs:
	@ echo GPATH: $(GPATH)
	@ echo VPATH: $(VPATH)
	@ echo SPATH: $(SPATH)

delv:
	@ echo LIBS: $(DV_LIBS)
	@ echo UTILS: $(DV_UTILS)

ifeq ($(NODEPS),)
-include $(DEPS)
endif
