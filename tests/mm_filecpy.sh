
ERROR ON
# ----------------------------------------------------------------------------
# Memory mapping

# map 
KMAPX FILECPY 4k rw lorem_xs.txt 0 @ma1
TOUCH @ma1 w
SHOW

# unmap
KUNMAP @ma1 4k
ERROR ENOMEM
TOUCH @ma1 w
ERROR ON

# protect (r- => rw)
KMAPX FILECPY 8k r- lorem_sm.txt 0 @ma2
TOUCH @ma2 r
ERROR ENOMEM
TOUCH @ma2 w
ERROR ON
KPROTECT @ma2 8k rw
TOUCH @ma2 w
KUNMAP @ma2 8k

# protect (rw => r-)
KMAPX FILECPY 8k rw lorem_sm.txt 0 @ma3
TOUCH @ma3 w
KPROTECT @ma3 8k r-
ERROR ENOMEM
TOUCH @ma3 w
ERROR ON
TOUCH @ma3 r
KUNMAP @ma3 8k

# protect split !?
KMAPX FILECPY 8k r- lorem_sm.txt 0 @ma4
KPROTECT @ma4+4k 4k rw
SHOW
TOUCH @ma4 r
TOUCH @ma4+4k w
ERROR ENOMEM
TOUCH @ma4 w
ERROR ON
KUNMAP @ma4 4k
SHOW
TOUCH @ma4+4k r
ERROR ENOMEM
TOUCH @ma4 r
ERROR ON
KUNMAP @ma4+4k 4k
SHOW

# testing clone
USPACE_CREATE @us1
MMAPX FILECPY 20k rw lorem_sm.txt 0 @ma5
TOUCH @ma5 r
TOUCH @ma5+4k rw
TOUCH @ma5+8k rw
SHOW

USPACE_CLONE @us2
SHOW
USPACE_SELECT @us1
SHOW

TOUCH @ma5+12k rw
TOUCH @ma5+4k rw
SHOW

USPACE_CLONE @us3
TOUCH @ma5+8k rw
SHOW

USPACE_SELECT @us2
TOUCH @ma5+4k rw
SHOW

USPACE_SELECT @us3
USPACE_CLOSE @us3

USPACE_SELECT @us1
TOUCH @ma5+4k r
TOUCH @ma5+12k rw

USPACE_SELECT @us2
SHOW
USPACE_CLOSE @us2

USPACE_SELECT @us1
TOUCH @ma5 rw
TOUCH @ma5+16k r

USPACE_SELECT @us1
USPACE_CLOSE @us1

# protect update
KMAPX FILECPY 8k rw lorem_sm.txt 0 @ma6
TOUCH @ma6 rw

KPROTECT @ma6 8K r
ERROR ENOMEM
TOUCH @ma6 rw
ERROR ON
TOUCH @ma6+4K r

KPROTECT @ma6 8K rw
TOUCH @ma6 r
TOUCH @ma6+4K rw
TOUCH @ma6 rw

KUNMAP @ma6 8K

# protect and copy-on-write
USPACE_CREATE @us4
MMAPX FILECPY 20k rw lorem_sm.txt 0 @ma7
TOUCH @ma7 rw
SHOW

USPACE_CLONE @us5
TOUCH @ma7+4k w
MPROTECT @ma7 20k r
ERROR ENOMEM
TOUCH @ma7 w
ERROR ON
SHOW

USPACE_CLONE @us6
SHOW
MPROTECT @ma7 20k rw
SHOW

TOUCH @ma7 rw
SHOW

USPACE_SELECT @us5
MPROTECT @ma7 20k rw
SHOW

USPACE_SELECT @us6
USPACE_CLOSE @us6
USPACE_SELECT @us5
USPACE_CLOSE @us5
USPACE_SELECT @us4
USPACE_CLOSE @us4


# cleanup
DEL @ma1
DEL @ma2
DEL @ma3
DEL @ma4
DEL @ma5
DEL @ma6
DEL @ma7
