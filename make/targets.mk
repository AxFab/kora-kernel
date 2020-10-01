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

deliveries: $(BINS) $(LIBS)

install: install-utilities install-shared install-headers install-pkgconfig

install-utilities: $(call fn_inst, $(BINS))

install-shared: $(call fn_inst, $(LIBS))

ifeq (,$(wildcard $(topdir)/$(PACKAGE).pc.in))
install-pkgconfig:
else
install-pkgconfig: $(gendir)/$(PACKAGE).pc
	$(S) mkdir -p $(prefix)/lib/pkgconfig
	$(S) cp -RpP -f $< $(prefix)/lib/pkgconfig/

$(gendir)/$(PACKAGE).pc: $(topdir)/$(PACKAGE).pc.in
	$(S) echo "prefix=${prefix}" > $@
	$(S) echo "version=${VERSION}" >> $@
	$(S) cat $< >> $@
endif


.PHONY: all install install-utilities install-shared install-headers install-pkgconfig

