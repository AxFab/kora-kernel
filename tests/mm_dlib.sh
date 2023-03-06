
ERROR ON
# ----------------------------------------------------------------------------
# Library mapping

# basic mapping
KDLIB bootrd/ata.ko
SHOW

KSYM ata_setup @a1
TOUCH @a1 r
SHOW
ERROR ENOMEM
TOUCH @a1 w
ERROR ON

KSYM ata_ino_ops @a2
TOUCH @a2 r
SHOW
TOUCH @a2 w
SHOW

KSYM inb @a3
TOUCH @a3 r
SHOW

DEL @a1
DEL @a2
DEL @a3



