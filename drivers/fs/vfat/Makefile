
vfat_LFLAGS += $(LFLAGS)
vfat_SRCS-y := $(wildcard $(topdir)/vfat/*.c)
$(eval $(call link_driver,vfat))

mkvfat_SRCS-y := $(topdir)/vfat/utils/format.c
$(eval $(call link_sbin,mkvfat))

