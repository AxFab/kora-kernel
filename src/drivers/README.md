# Kora OS Drivers


 - **File Systems**
  - ext2. Old version of Linux file system.
  - fat. Most common file system, support FAT12, FAT16 and FAT32.
  - isofs. ISO 9660, this file system is the one used on cdrom.
 - **Disk drivers**
  - IDE ATA / ATAPI. Look for hard-drive and cdrom (`sdA, sdB, sdC, sdD`).
  - GPT. Is able to create logical volumes from other block devices (`sdA1`).
  - IMG Disk. Is an hosted simulation, works only on user-mode. Create device from regular files.
 - **Input devices**
  - PS/2. Handle standard keyboard and mouse connected to PS/2 port.
 - **Network card**
  - E1000 Intel PRO/1000 Ethernet controller. (PCI device)
 - **Media** (audio and video)
  - ac97. Audio card (PCI device)
  - vga. Standard VGA compatible video card (PCI device).
 - **Miscalenous**
  - PCI. This driver is able to identify PCI devices on PC architecture.
  - VBOX. VirtualBox Guest additions. (PCI device)


