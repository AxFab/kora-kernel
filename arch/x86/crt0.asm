use32

global _start
extern __libc_init, main, exit

_start:
    mov ebp, esp
    and esp, ~0xF
    call __libc_init
    xor eax, eax
    mov [esp + 4], eax
    mov [esp + 8], eax
    mov [esp + 12], eax
    call main
    mov [esp + 4], eax
    call exit
    jmp $
