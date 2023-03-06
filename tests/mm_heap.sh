
# ----------------------------------------------------------------------------
# Heap & Stack
# map
KMAP HEAP 16k rw @mh1
TOUCH @mh1 w
TOUCH @mh1+4k r

KMAP STACK 16k rw @ms1
TOUCH @ms1 w
SHOW

# unmap
ERROR EINVAL
KUNMAP @mh1 4k
ERROR ON
KUNMAP @mh1 16k
ERROR ENOMEM
TOUCH @mh1 w
ERROR ON

KUNMAP @ms1 16k

# TODO protect, split forbidden
# TODO clone work like anonymous
# TODO clone for stack !?

DEL @mh1
DEL @ms1
