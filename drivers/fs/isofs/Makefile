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
name = isofs
topdir ?= $(shell readlink -f $(dir $(word 1,$(MAKEFILE_LIST)))/..)

include $(topdir)/_scripts/make_vars.mk

src-y = $(wildcard $(topdir)/isofs/*.c)

$(name): $(libdir)/$(name).km
include $(topdir)/_scripts/make_rules.mk
include $(topdir)/_scripts/make_kmodule.mk
