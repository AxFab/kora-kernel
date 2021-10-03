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

define link_driver
DRVS += $(libdir)/$(1).ko
INSTALL_DRVS += $(prefix)/boot/mods/$(1).ko
$(1): $(libdir)/$(1).ko $(libdir)/lk$(1).a
$(libdir)/$(1).ko: $(call fn_objs,$(1)_SRCS)
	$(S) mkdir -p $$(dir $$@)
	$(Q) echo "    LD  $$@"
	$(V) $(LD) -shared -o $$@ $$^ $($(1)_LFLAGS)
$(libdir)/lk$(1).a: $(call fn_objs,$(1)_SRCS)
	$(S) mkdir -p $$(dir $$@)
	$(Q) echo "    AR  $$@"
	$(V) $(AR) rc $$@ $$^
$(prefix)/boot/mods/$(1).ko: $(libdir)/$(1).ko
	$(S) mkdir -p $$(dir $$@)
	$(Q) echo "    INSTALL  $$@"
	$(V) $(INSTALL) $$< $$@
.PHONY:$(1)
endef

define link_sbin
BINS += $(bindir)/$(1)
INSTALL_BINS += $(prefix)/sbin/$(1)
$(1): $(bindir)/$(1)
$(bindir)/$(1): $(call fn_objs,$(1)_SRCS)
	$(S) mkdir -p $$(dir $$@)
	$(Q) echo "    LD  $$@"
	$(V) $(CC) -o $$@ $$^ $($(1)_LFLAGS)
$(prefix)/sbin/$(1): $(bindir)/$(1)
	$(S) mkdir -p $$(dir $$@)
	$(Q) echo "    INSTALL  $$@"
	$(V) $(INSTALL) $$< $$@
endef

