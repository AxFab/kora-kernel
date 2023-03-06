
ERROR ON
# ----------------------------------------------------------------------------
# File mapping
USPACE_CREATE @us1

# map
MMAPX FILE 20k r lorem_sm.txt 0 @mf1
TOUCH @mf1 r
TOUCH @mf1+4k r
TOUCH @mf1+8k r
TOUCH @mf1+12k r
TOUCH @mf1+16k r
SHOW

MMAPX FILE 4k rw lorem_xs.txt 0 @mf2
TOUCH @mf2 rw
SHOW

# unmap
MUNMAP @mf2 4k
SHOW

MUNMAP @mf1+8k 12k
MUNMAP @mf1 8k
SHOW

# protect
MMAPX FILE 12k rw lorem_sm.txt 0 @mf3
TOUCH @mf3 rw
MPROTECT @mf3 12k r
SHOW
ERROR ENOMEM
TOUCH @mf3+4k w
ERROR ON

MUNMAP @mf3 12k

# clone
MMAPX FILE 20k r lorem_sm.txt 0 @mf4
MMAPX FILE 4k rw lorem_xs.txt 0 @mf5
TOUCH @mf4 r 
TOUCH @mf5 w
SHOW

USPACE_CLONE @us2
SHOW
TOUCH @mf4 r 
TOUCH @mf5 w
SHOW

USPACE_SELECT @us1
USPACE_CLOSE @us1
USPACE_SELECT @us2
USPACE_CLOSE @us2

DEL @mf1
DEL @mf2
DEL @mf3
DEL @mf4
DEL @mf5
