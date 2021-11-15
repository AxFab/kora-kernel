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
$(outdir)/%.o: $(topdir)/%.c
	$(S) mkdir -p $(dir $@)
	$(Q) echo "    CC  $<"
	$(V) $(CC) -c -o $@ $< $(CFLAGS)

$(outdir)/%.d: $(topdir)/%.c
	$(S) mkdir -p $(dir $@)
	$(Q) echo "    CM  $<"
	$(V) $(CC) -M $< $(CFLAGS) | sed "s%$(notdir $(@:.d=.o))%$(@:.d=.o)%" > $@

# -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

define comp_source
$(outdir)/$(1)/%.o: $(topdir)/%.c
	$(S) mkdir -p $$(dir $$@)
	$(Q) echo "    CC  $$<"
	$(V) $(CC) -c -o $$@ $$< $($(2))

$(outdir)/$(1)/%.d: $(topdir)/%.c
	$(S) mkdir -p $$(dir $$@)
	$(Q) echo "    CM  $$<"
	$(V) $(CC) -M $$< $($(2)) | sed "s%$$(notdir $$(@:.d=.o))%$$(@:.d=.o)%" > $$@
endef

define link_shared
LIBS += $(libdir)/lib$(1).so
lib$(1): $(libdir)/lib$(1).so
install-lib$(1): $(prefix)/lib/lib$(1).so
$(libdir)/lib$(1).so: $(call fn_objs,$(2),$(4))
	$(S) mkdir -p $$(dir $$@)
	$(Q) echo "    LD  $$@"
	$(V) $(LDC) -shared -o $$@ $$^ $($(3))
endef

define link_bin
BINS += $(bindir)/$(1)
$(1): $(bindir)/$(1)
install-$(1): $(prefix)/bin/$(1)
$(bindir)/$(1): $(call fn_objs,$(2),$(4))
	$(S) mkdir -p $$(dir $$@)
	$(Q) echo "    LD  $$@"
	$(V) $(LDC) -o $$@ $$^ $($(3))
endef

# -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
clean:
	$(V) rm -rf $(outdir)
	$(V) rm -rf $(libdir)
	$(V) rm -rf $(bindir)

$(prefix)/lib/%: $(libdir)/%
	$(S) mkdir -p $(dir $@)
	$(Q) echo "    INSTALL  $@"
	$(V) $(INSTALL) $< $@

$(prefix)/bin/%: $(bindir)/%
	$(S) mkdir -p $(dir $@)
	$(Q) echo "    INSTALL  $@"
	$(V) $(INSTALL) $< $@



.PHONY: all clean deliveries
