ERROR ON

CREATE vm0

# -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
# Create heap 
HEAP 64k kh1

# Test page fault on heap
TOUCH @kh1 w
TOUCH @kh1+4k r
TOUCH @kh1+4k w
SHOW p8K,v64k,s0

HEAP 16k kh2
TOUCH @kh2 w 
TOUCH @kh2+4k r 
TOUCH @kh2+8k w
SHOW p20K,v80k,s0

# Protection change are not allowed
ERROR EPERM
PROTECT @kh1 4M r
ERROR ON

# Shared heap are not allowed
# MAP 2M kh3 H rw.s

# We can unmap heap partial or complete
UNMAP @kh2 16k
SHOW p8K,v64k,s0

# Stack work the same
STACK 12k ks1
SHOW p8K,v76k,s0

# Stack can't be just unmap (!??)



# On VM clone heaps (and stacks) goes on COW
CLONE vm1

# Test Heap COW (read PF)
TOUCH @kh1 r


# Test Heap COW (write PF) 
TOUCH @kh1+4k w

SHOW

# Clone a clone put all back into cow
# Close on of the VM doesn't impact the other
# Close last with both shared and private VMAs


# Pipe test similar to heap without clone
# Pipe can't be created into user vm

# Phys test similar Pipe but auto resolve,
# Mprotect always allow to set the write flags

# Private anon are like heap (+cow)
# mprotect, is complex

# Shared anon


# Private file

# Shared file

# Code

# Some elf reading tests


# CREATE vm0
# STACK 1M
# EXEC libc.so 20k 12k
# EXEC libz.so 12K 4K
# EXEC krish 24k 16k
# SHOW
# TOUCH 0x04FFFF0 w
# TOUCH 0x02000000 x # libc.so
# HEAP 8m
# TOUCH 0x00500000 w # Heap.0
# TOUCH 0x00501000 w # Heap.1
# SHOW
# TOUCH 0x02005000 w # libc.so .data
# SHOW
