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

$(outdir)/dr/%.o: $(topdir)/%.c
	$(S) mkdir -p $(dir $@)
	$(Q) echo "    CC  $<"
	$(V) $(CC) -c -o $@ $< $(CFLAGS_dr)

$(outdir)/dr/%.d: $(topdir)/%.c
	$(S) mkdir -p $(dir $@)
	$(Q) echo "    CM  $<"
	$(V) $(CC) -M $< $(CFLAGS_dr) | sed "s%$(notdir $(@:.d=.o))%$(@:.d=.o)%" > $@


define link_driver
DRVS += $(libdir)/$(1).ko
INSTALL_DRVS += $(prefix)/boot/mods/$(1).ko
$(1): $(libdir)/$(1).ko $(libdir)/lk$(1).a
$(libdir)/$(1).ko: $(call fn_objs2,$(1)_SRCS,dr)
	$(S) mkdir -p $$(dir $$@)
	$(Q) echo "    LD  $$@"
	$(V) $(LD) -shared -o $$@ $$^ $($(1)_LFLAGS_dr)
$(libdir)/lk$(1).a: $(call fn_objs2,$(1)_SRCS,dr)
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
$(bindir)/$(1): $(call fn_objs2,$(1)_SRCS,dr)
	$(S) mkdir -p $$(dir $$@)
	$(Q) echo "    LD  $$@"
	$(V) $(CC) -o $$@ $$^ $($(1)_LFLAGS_dr)
$(prefix)/sbin/$(1): $(bindir)/$(1)
	$(S) mkdir -p $$(dir $$@)
	$(Q) echo "    INSTALL  $$@"
	$(V) $(INSTALL) $$< $$@
endef

