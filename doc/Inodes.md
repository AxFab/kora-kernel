

## Creation of an inode

  After `vfs_create` (or `vfs_mkdir`, `vfs_symlink`, `vfs_mkfifo`) or
  `vfs_lookup` the return inode must be released by a call to `vfs_close`.
  s

## Closing an inode

  Inode have a counter that's incremented each time the inode is reference
  somwhere. Any task which will try to destroy the inode structure will have
  to check the user counter is down to zero. In any other case, the inode
  structure can't be destroyed.


