
vfat_LFLAGS += $(LFLAGS)
vfat_SRCS := $(wildcard $(topdir)/drivers/fs/vfat/*.c)
$(eval $(call link_driver,vfat))

mkvfat_SRCS := $(topdir)/drivers/fs/vfat/utils/format.c
$(eval $(call link_sbin,mkvfat))

