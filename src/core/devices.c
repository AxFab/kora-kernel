#include <kernel/core.h>
#include <kernel/vfs.h>
#include <kernel/sys/inode.h>
#include <errno.h>
#include <string.h>

typedef struct device device_t;

struct device {

  char id[16];
  const char *vendor;
  const char *class;
  const char *device;
  const char *name;
  inode_t *ino;


  uint32_t dma_phys;
  uint32_t dma_length;
  void *mmio;
  int irq;
  int io_base;
};

int vfs_mkdev(const char *name, inode_t *ino, const char* vendor, const char* class, const char* device, unsigned char id[16])
{
  int i;
  if (name == NULL || ino == NULL) {
    errno = EINVAL;
    return -1;
  }

  device_t *dev = (device_t*)kalloc(sizeof(device_t));
  dev->vendor = strdup(vendor);
  dev->class = strdup(class);
  dev->device = strdup(device);

  if (id != NULL) { // Check ID is not zero
    memcpy(dev->id, id, 16);
  } else {
    for (i = 0; i < 16; ++i) {
      dev->id[i] = rand() & 0xFF;
    }
  }

  dev->ino = vfs_open(ino);

  // Create directory entry as '/dev/{name}'
  if (kVFS.devIno == NULL) {
    kVFS.devIno = vfs_mkdir(kVFS.root, "dev", S_IFDIR | 0755);
    if (kVFS.devIno == NULL) {
      kpanic("Unable to create 'dev' repository.");
    }
  }

  int ret = vfs_mknod(kVFS.devIno, name, ino);
  if (ret) {
    return -1;
  }

  dev->name = strdup(name);

  // TODO -- Use id to check if we know the device.
  errno = 0;
  return 0;
}

