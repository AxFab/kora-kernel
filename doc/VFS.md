
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



