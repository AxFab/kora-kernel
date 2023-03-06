
ERROR ON
# ----------------------------------------------------------------------------
# Pipe
# Map & write
KMAP PIPE 8k - @mp1
TOUCH @mp1 w
TOUCH @mp1+4k w
SHOW

# Remap (unmap)
KMAP PIPE 16k - @mp2
TOUCH @mp2 w
TOUCH @mp2+4k w
KUNMAP @mp1 8k
SHOW

# Resolving...
TOUCH @mp2+8 r
TOUCH @mp2+12 rw
SHOW

# Can't protect or clone
ERROR EINVAL
KPROTECT @mp2, 8k r
KPROTECT @mp2, 16k r
KUNMAP @mp2 12k
ERROR ON

KUNMAP @mp2 16k
SHOW
DEL @mp1
DEL @mp2

# ----------------------------------------------------------------------------
# Physique & DMA
ERROR EINVAL
KMAPX PHYS 1M ru - 0xcafe00 @mp1
ERROR ON
SHOW

#KMAPX PHYS 4k r - 0 @mp2
#TOUCH @mp2 r
#MMUREAD @mp2 @pg1
#KUNMAP @mp2 4k
#MMURELEASE @pg1

KMAPX PHYS 128k rwu - 0xbad00000 @mp1
TOUCH @mp1 r
KUNMAP @mp1 128k
SHOW

DEL @mp1
