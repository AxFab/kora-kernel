# Structures

## CORE - acl_t
## CORE - advent_t
## CORE - evmsg_t
## CORE - kapi_t
## CORE - kmodule_t
## CORE - masterclock_t
## CORE - syscall_info_t

------------------------------------------------------------------------------

## NET - dhcp_info_t
## NET - eth_header_t
## NET - eth_info_t
## NET - icmp_info_t
## NET - ifnet_t
## NET - ip4_frame_t
## NET - ip4_info_t
## NET - ip4_master_t
## NET - ip4_route_t
## NET - net_ops_t
## NET - netstack_t
## NET - nproto_t
## NET - skb_t
## NET - socket_t

------------------------------------------------------------------------------

## TASK - dyndep_t
## TASK - dynlib_t
## TASK - dynrel_t
## TASK - dynsec_t
## TASK - dynsym_t
## TASK - fstream_t
## TASK - proc_t
## TASK - process_t
## TASK - scheduler_t
## TASK - streamset_t
## TASK - task_param_t
## TASK - task_t

------------------------------------------------------------------------------

## VFS - bkmap_t

 Private strucutre at usage for file system drivers. It's used to ease query
 of a disk sector while handling page boundaries.

## VFS - device_t

 The device is a structure that composed an inode. Device strucutre are
 created while `vfs_inode` is called without device. This structure can be
 field by a driver to store information string.

 > It currently contains also the underlying file for block based file system
 however this cause issue for device release and might evole in near future.

## VFS - diterator_t
## VFS - fl_ops_t
## VFS - fsnode_t

 The fsnode is used by the kernel to compose the file tree. One fsnode match
 on path, but remember that several fsnode can point to the same file or inode.

 The fsnode are handle by a RCU+LRU mechanisms, but note that scavenge policy
 haven't been properly defined yet.



## VFS - ino_ops_t
## VFS - inode_t

 The inode is the kernel representation of a file. Only one instance can exist
 per file, which can be identify by the device and a unique per device inode
 number. Their livespan is ruled by a RCU mechanisms trigger on open/close
 operations.


 To create an inode drivers must use the `vfs_inode` method which take the
 inode number, the current device, the type of the file and a structure for
 inode operations `ino_ops_t`.

 > Note that file types are not the unix ones but are more detailed on kora.


## VFS - path_t

 This structure is only used internaly and should always keep a small
 livespan.

 It's a list of path component to request a file. The kernel should not keep
 `path_t` but store a `fsnode_t` unstead.

## VFS - vfs_t

------------------------------------------------------------------------------

## VM - kMmu !!
## VM - mspace_t

 > Might be renamed `vms_t`

 This structure hold the information about a _virtual memory space_. It's
 defined by a lower and upper bound and a collection of `vma_t`structures.
 The `vma_t` are stored on a tree for best performance.

 The `vms_t` hold information about the current memory context to help
 resolving missing pages. Each space describe a different memory context.

 The kernel hold N structure which usually share the same boundaries, and one
 extra for the dedicated usage of the kernel.

 All `vms_t` strucure contains counter of pages used or shared by this
 particular memory context, which allow for memory monitoring.

 Finally `vms_t` instance are tracked by an RCU mechanisms which trigger the structure destruction on the last close operation.

## VM - vma_t



------------------------------------------------------------------------------

