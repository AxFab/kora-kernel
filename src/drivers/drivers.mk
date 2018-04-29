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
drvdir = $(srcdir)/drivers

drv_src-y += $(wildcard $(drvdir)/disk/ata/*.c)
drv_src-y += $(wildcard $(drvdir)/disk/gpt/*.c)
# drv_src-y += $(wildcard $(drvdir)/disk/imgdk/*.c)

# drv_src-y += $(wildcard $(drvdir)/fs/ext2/*.c)
drv_src-y += $(wildcard $(drvdir)/fs/fat/*.c)
drv_src-y += $(wildcard $(drvdir)/fs/isofs/*.c)

drv_src-y += $(wildcard $(drvdir)/input/ps2/*.c)

drv_src-y += $(wildcard $(drvdir)/media/ac97/*.c)
drv_src-y += $(wildcard $(drvdir)/media/vga/*.c)

drv_src-y += $(wildcard $(drvdir)/misc/pci/*.c)
drv_src-y += $(wildcard $(drvdir)/misc/vbox/*.c)

drv_src-y += $(wildcard $(drvdir)/net/e1000/*.c)
