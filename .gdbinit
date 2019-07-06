target remote localhost:1234
b kpanic
b __assert_fail
b irq_fault
b kernel_start
c

# add-symbol-file ../build/lib/libc.so 0x12cb0340
# add-symbol-file ../build/lib/libgfx.so 0x1232da90
# add-symbol-file ../build/bin/krish 0x8048ec0
