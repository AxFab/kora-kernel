# Virtual File System

The virtual file system is an abstraction layer that provide access to _devices and files_ in a common way, regardless of the way datas are stored.
The VFS enables various file systems to coexist but also to interoperate with each others.

The VFS is a standalone library and can be easily tested isolated form the rest of the kernel as long as some driver might provide access to some devices.
All the sources are placed at `src/core/vfs/*` and ABI is defined on the unique `include/kernel/vfs.h` header file.

This interface is used to access one single type of structure, the inode (`inode_t`. However it will not work without _drivers and file systems_, most of them being provided as kernel module.
The VFS regroup as inodes a lot of various generic files, but several types of files coexist.

## Devices

There is several type of devices, but there mostly used to communicate with external component of hardware like hard-drives or keyboard.
Each devices might be different but we can classified those by their behaviour into 4 categories:

 - Block devices
 - Character devices
 - Network devices
 - Video devices

> Note that video devices are not present on either Unix or Linux, even if at some point Linux experimented with frame buffers.
> It's not hard in stone, but I try here to create some generic access to interact with both screens and cameras.

Most of the time we need a driver to handle one or a couple of devices.
Such kernel module will try to detect hardware during setup and then will create a new device.
To know more, look as section __device drivers__.

Block devices are primarly used as data storade like hard-disk, cdrom, memory cards.
A block of device is defined as a large block of randomly accessible data.
This mean that data can be referenced by addresses and you don't have to read the all data in a sequencial order.
Block devices have usually a fixed size, however nothing prevent a device to play around this rule.
As they often serve as storage, they are often mounted as a file system.

> The most common block device is the hard drive. Second is the volume. A volume us usually a virtual chunk of a physical drive. However the handling of devices build over other devices is not handle well on the kernel yet. At this moment MGR and GPT are supported but only hard-pluged into the VFS after the creation of each block device. This is not ideal architecture as new partitions rule can't be easly plugged into the kernel. This is under study as it might be handy for software RAID or crytographic modules.

Character devices are not addressable, providing only stream of data. This include every event based devices like keyboard, mise but also printers and pseudo devices.

Network devices are particular devices. It allow to send or received isolated
chunk of data, called _socket buffers or network packets_.
The network devices are usually access through the socket interface.

A video device is a combinaison of a couple of 2D-bitmap frames.
The frames are usually cached made during the building of the next frame.
Frames can be mapped similarly to block devices.
However frames are send or received to follow a certain time stream which make this device a bastard between block and character devices.

## File System

A file system is a collection of files stored into a hierarchy of directories.

Directories are containers for others files and can be used as access point.
A directory must also have parent unless for the root of the file system.

The VFS doesn't assume that the parent of a directory is the directory that contains it. This allow to support hard links.

File system usually contains mostly regular-files which behave like block
device for io operations. But they also provide some meta-data that must be
handle by the inode itself.
Note that regular-files can be mounted as for block device.

Some file system also allow the use of symlinks. A symlink is a small file that contains a URL to reference another file.

A directory can contains any sort of files, however almost none will at the exception of `devfs` or `procfs`.

## Other files

Other files are usualy created as memory-only and serve as API connectors.

The most knonwn of those is the pipe or fifo that can be used to transfert data from one program to another.
A pipe is always mapped into kernel memory and will create two cursors, one for input, the other for output of data.

The socket allow to transfert data from one machine to another.
This one depends entierly on the network stack to split a stream of data into small transportable packets and to find its destination.

A new file that has no equivalent into Unix world is the surface. The surface allow to create a video stream. Most of them will be used directly by the windows manager.

> In the futur, we can hope that some surface might be created by graphical card for accelerated rendering.
> For the moment this one reside into kernel memory but can be mapped by user application.

Last but not least is the terminal or tty. This one is often seen as legacy on Linux, however I decide it can be handy to have a fast text display mechanism for easy debugging.
The tty is a complex structure that contains a fifo for output, a fifo for input and a surface for display.






## Inodes

An inode is a representation of a file. Is it used for all IO operations.
There is several type of inodes. First

First operation to understand is the the way to manipulate a file system tree.

```
inode_t *root = // ... You need the root of a file system to manipulate the VFS.

/* Look for an inode */
inode_t *dir_ino = vfs_search(root, root, "usr/local/etc");

/* Create an empty inode */
inode_t *cfg_ino = vfs_create(dir_ino, "config.ini", S_IFREG | 0644, uid, gid);
inode_t *sym_ino = vfs_symlink(dir_ino, "global.ini", "/usr/etc/global.ini");
inode_t *glb_ino = vfs_follow(sym_ino);

/* Link an inode (If supported) */
int ret = vfs_link(root, ".config", cfg_ino);


/* Update inode meta-data */
int ret = vfs_chown(dir_ino, uid, gid);
int ret = vfs_chmod(dir_ino, 0755);
int ret = vfs_chtimes(dir_ino, &time_spec, INO_ATIME | INO_MTIME | INO_CTIME);

/* Resize inode */
int ret = vfs_truncate(cfg_ino, 4 * _Mib_);

/* Delete an inode */
int ret = vfs_unlink(dir_ino, "config.ini");

```




## Device drivers


```
/*  */
int vfs_mkdev(cstr name, inode_t *ino, cstr vendor, cstr class, cstr dname, uint8_t uid[16]);
```

------------
------------
------------
------------




# Virtual File System


## Inode Live time

  The inode is a structure used to reference, on the kernel, a POSIX file present in an external device.

```
vfs_open()
vfs_close()
```




## FIFO Size

  The capacity of a pipe buffer varies across systems (and can even vary on the same system).

  Mac OS X, for example, uses a capacity of 16Kb by default, but can switch to 64Kb large write are made to the pipe, or will switch to a capacity of a single system page if too much kernel memory is already being used by pipe buffers.
  Since these are from FreeBSD, the same behavior may happen there, too).

  On Linux, man page says that pipe capacity is 64Kb since Linux 2.6.11 and a single system page prior to that.
  The code seems to use 16 system pages, but the buffer for each pipe can be adjusted via a fcntl on the pipe up to a maximum capacity which defaults to 1Mb, but can be changed via /proc/sys/fs/pipe-max-size.

  KoraOS Will default to a single page. The data loop around the page when possible however it will not block until the fifo automatically extends up to 64Kb.





## Creation of an inode

  After `vfs_create` (or `vfs_mkdir`, `vfs_symlink`, `vfs_mkfifo`) or
  `vfs_lookup` the return inode must be released by a call to `vfs_close`.
  s

## Closing an inode

  Inode have a counter that's incremented each time the inode is reference
  somwhere. Any task which will try to destroy the inode structure will have
  to check the user counter is down to zero. In any other case, the inode
  structure can't be destroyed.


