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
DRVS += $(1)
$(1): $(libdir)/$(1).km $(libdir)/lk$(1).a
$(libdir)/$(1).km: $(call fn_objs,$(1)_SRCS-y)
	$(S) mkdir -p $$(dir $$@)
	$(Q) echo "    LD  $$@"
	$(V) $(LD) -shared -o $$@ $$^ $($(1)_LFLAGS)
$(libdir)/lk$(1).a: $(call fn_objs,$(1)_SRCS-y)
	$(S) mkdir -p $$(dir $$@)
	$(Q) echo "    AR  $$@"
	$(V) $(AR) rc $$@ $$^
install-$(1): $(prefix)/lib/drivers/$(1).km
$(prefix)/lib/drivers/$(1).km: $(libdir)/$(1).km
	$(S) mkdir -p $$(dir $$@))
	$(V) $(INSTALL) $$< $$@
.PHONY:$(1)
endef

define link_sbin
BINS += $(1)
$(1): $(bindir)/$(1)
$(bindir)/$(1): $(call fn_objs,$(1)_SRCS-y)
	$(S) mkdir -p $$(dir $$@)
	$(Q) echo "    LD  $$@"
	$(V) $(CC) -o $$@ $$^ $($(1)_LFLAGS)
install-$(1): $(prefix)/sbin/$(1)
$(prefix)/sbin/$(1): $(bindir)/$(1)
	$(S) mkdir -p $$(dir $$@)
	$(V) $(INSTALL) $$< $$@
endef

