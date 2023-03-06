
ERROR ON

START 64M 64M 6K

INCLUDE mm_misc.sh
INCLUDE mm_anon.sh
INCLUDE mm_heap.sh
INCLUDE mm_file.sh
INCLUDE mm_filecpy.sh
INCLUDE mm_dlib.sh


# ----------------------------------------------------------------------------
# Manage user virtual memory space
# Consumme lots of pages
# Bad page-faults
# Library resolve / Fixed & Hint address / Copy-on-write

ERROR OFF
QUIT
EXIT

