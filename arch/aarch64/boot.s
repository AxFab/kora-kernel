// To keep this in the first portion of the binary.
.section ".text.boot"

// Make _start global.
.global _start
.global start

// Entry point for the kernel.
// r15 -> should begin execution at 0x8000.
// r0 -> 0x00000000
// r1 -> 0x00000C42
// r2 -> 0x00000100 - start of ATAGS
// preserve these registers as argument for kernel_main
_start:
start:
    # cpsid if

    // Shut off extra core
    mrc p15, 0, r5, c1, c0, 0
    # and r5, r5, #3
    # cmp r5, #0
    # bne halt

    // Setup the stack.
    ldr r5, _start
    mov sp, r5

    // Clear out bss.
    ldr r4, =__bss_start
    ldr r9, =__bss_end
    mov r5, #0
    mov r6, #0
    mov r7, #0
    mov r8, #0
    b       2f

1:
    // store multiple at r4.
    stmia r4!, {r5-r8}

    // If we are still below bss_end, loop.
2:
    cmp r4, r9
    blo 1b

    // Call kernel_main
    ldr r3, =kernel_main
    blx r3

    // halt
halt:
    wfe
    b halt
