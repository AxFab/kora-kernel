
vfat_LFLAGS += $(LFLAGS)
vfat_SRCS := $(wildcard $(topdir)/vfat/*.c)
$(eval $(call link_driver,vfat))

mkvfat_SRCS := $(topdir)/vfat/utils/format.c
$(eval $(call link_sbin,mkvfat))

